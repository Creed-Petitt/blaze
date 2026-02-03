package cmd

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/fsnotify/fsnotify"
	"github.com/spf13/cobra"
)

var devRelease bool
var devWatch bool

var devCmd = &cobra.Command{
	Use:   "dev",
	Short: "Start the fullstack development environment",
	Run: func(cmd *cobra.Command, args []string) {
		runDev()
	},
}

func init() {
	rootCmd.AddCommand(devCmd)
	devCmd.Flags().BoolVarP(&devRelease, "release", "r", false, "Run in Release mode (Optimized)")
	devCmd.Flags().BoolVarP(&devWatch, "watch", "w", false, "Enable Hot Reload")
}

func runDev() {
	if _, err := os.Stat("frontend/package.json"); os.IsNotExist(err) {
		fmt.Println(blueStyle.Render("Error: No frontend found (missing frontend/package.json)."))
		return
	}

	// Auto-Install Dependencies
	if _, err := os.Stat("frontend/node_modules"); os.IsNotExist(err) {
		err := RunTaskWithSpinner("Installing frontend dependencies...", func() error {
			installCmd := exec.Command("npm", "install")
			installCmd.Dir = "frontend"
			// Silence output unless error
			if output, err := installCmd.CombinedOutput(); err != nil {
				return fmt.Errorf("npm install failed:\n%s", output)
			}
			return nil
		})

		if err != nil {
			fmt.Printf("\n%v\n", err)
			return
		}
		fmt.Println() // Spacer
	}

	projectName := getProjectName()
	backendPath := fmt.Sprintf("./build/%s", projectName)

	// 1. Initial Build
	if err := RunBlazeBuild(devRelease, true); err != nil {
		fmt.Println(err)
		return
	}

	// 2. Start Services
	var backendCmd *exec.Cmd
	var frontendCmd *exec.Cmd

	// Default to standard Vite port
	frontendUrl := "http://localhost:5173"

	// Start Frontend
	frontendCmd = exec.Command("npm", "run", "dev")
	frontendCmd.Dir = "frontend"

	// Set Process Group ID to kill children later
	frontendCmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}

	frontendOut, _ := frontendCmd.StdoutPipe()
	frontendErr, _ := frontendCmd.StderrPipe()

	if err := frontendCmd.Start(); err != nil {
		fmt.Printf("Error starting frontend: %v\n", err)
		return
	}

	// Monitor Frontend Output for Port Changes (Non-blocking)
	go func() {
		parse := func(r io.Reader) {
			scanner := bufio.NewScanner(r)
			for scanner.Scan() {
				line := scanner.Text()
				// Look for "Local: http://..."
				if strings.Contains(line, "Local:") || strings.Contains(line, "➜") {
					if strings.Contains(line, "http") {
						parts := strings.Fields(line)
						for _, p := range parts {
							if strings.HasPrefix(p, "http") {
								// If Vite moved ports (e.g. 5174), we could update UI here
							}
						}
					}
				}
			}
		}
		go parse(frontendOut)
		go parse(frontendErr)
	}()

	// Dashboard
	printDashboard := func() {
		fmt.Print("\033[H\033[2J") // Clear screen
		fmt.Println(blueStyle.Render(fmt.Sprintf("\n  🚀 Blaze Fullstack Server (%s)\n", projectName)))
		fmt.Printf("  Backend:  %s\n", greenStyle.Render("http://localhost:8080"))
		fmt.Printf("  Frontend: %s\n", greenStyle.Render(frontendUrl))
		fmt.Printf("  Docs:     %s\n\n", dimStyle.Render("http://localhost:8080/docs"))

		mode := "Debug"
		if devRelease {
			mode = "Release"
		}
		fmt.Printf("  [Build: %s]\n", orangeStyle.Render(mode))
		if devWatch {
			fmt.Println(dimStyle.Render("  [Hot Reload Active] Watching for changes..."))
		} else {
			fmt.Println(dimStyle.Render("  [Static Mode] (Press Ctrl+C to stop)"))
		}
	}

	// Show initial dashboard immediately
	printDashboard()

	// Start Backend
	var buildLock sync.Mutex
	isFirstRun := true

	restartBackend := func() {
		if !buildLock.TryLock() {
			return
		}
		defer buildLock.Unlock()

		if backendCmd != nil && backendCmd.Process != nil {
			// Kill the process group to ensure children die
			syscall.Kill(-backendCmd.Process.Pid, syscall.SIGKILL)
		}

		if !isFirstRun {
			fmt.Print(dimStyle.Render("\n  • Rebuilding..."))
			if err := RunBlazeBuild(devRelease, false); err != nil {
				fmt.Printf("\n  %s\n", orangeStyle.Render("Build Failed! Waiting for fix..."))
				return
			}
			printDashboard()
		}
		isFirstRun = false

		backendCmd = exec.Command(backendPath)
		backendCmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}

		// Pipe outputs directly
		backendCmd.Stdout = os.Stdout
		backendCmd.Stderr = os.Stderr

		backendCmd.Start()
	}

	restartBackend()

	// Setup Signal Handling for Cleanup
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	// Handle Shutdown Cleanup
	cleanup := func() {
		if backendCmd != nil && backendCmd.Process != nil {
			syscall.Kill(-backendCmd.Process.Pid, syscall.SIGKILL)
		}
		if frontendCmd != nil && frontendCmd.Process != nil {
			syscall.Kill(-frontendCmd.Process.Pid, syscall.SIGKILL)
		}
	}
	defer cleanup()

	if !devWatch {
		// Wait for signal
		<-sigChan
		return
	}

	// Watcher
	watcher, _ := fsnotify.NewWatcher()
	defer watcher.Close()
	filepath.Walk("src", func(path string, info os.FileInfo, err error) error {
		if info != nil && info.IsDir() {
			watcher.Add(path)
		}
		return nil
	})
	filepath.Walk("include", func(path string, info os.FileInfo, err error) error {
		if info != nil && info.IsDir() {
			watcher.Add(path)
		}
		return nil
	})

	var timer *time.Timer
	for {
		select {
		case <-sigChan:
			return
		case event, ok := <-watcher.Events:
			if !ok {
				return
			}
			if event.Op&fsnotify.Write == fsnotify.Write {
				if timer != nil {
					timer.Stop()
				}
				timer = time.AfterFunc(100*time.Millisecond, func() {
					go restartBackend()
				})
			}
		case <-watcher.Errors:
			return
		}
	}
}
