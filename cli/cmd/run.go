package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"sync"
	"syscall"
	"time"

	"github.com/fsnotify/fsnotify"
	"github.com/spf13/cobra"
)

var runWatch bool
var runRelease bool

var runCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the Blaze project",
	Run: func(cmd *cobra.Command, args []string) {
		if runWatch {
			runWithWatch(runRelease)
		} else {
			runStatic(runRelease)
		}
	},
}

func init() {
	rootCmd.AddCommand(runCmd)
	runCmd.Flags().BoolVarP(&runWatch, "watch", "w", false, "Watch for changes and hot-reload")
	runCmd.Flags().BoolVarP(&runRelease, "release", "r", false, "Build in Release mode")
}

func runStatic(release bool) {
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println(blueStyle.Render("Error: No Blaze project found."))
		return
	}

	if err := RunBlazeBuild(release, true); err != nil {
		fmt.Println(err)
		return
	}

	projectName := getProjectName()

	fmt.Print("\033[H\033[2J") // Clear screen
	fmt.Println(blueStyle.Render(fmt.Sprintf("\n  Blaze Server (%s)\n", projectName)))
	fmt.Printf("  Backend: %s\n", greenStyle.Render("http://localhost:8080"))
	fmt.Printf("  Docs:    %s\n\n", dimStyle.Render("http://localhost:8080/docs"))

	mode := "Debug"
	if release {
		mode = "Release"
	}
	fmt.Printf("  [Build: %s]\n", orangeStyle.Render(mode))
	fmt.Println(dimStyle.Render("  (Press Ctrl+C to stop)"))

	runCmd := exec.Command("./build/" + projectName)
	runCmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}
	runCmd.Stdout = os.Stdout
	runCmd.Stderr = os.Stderr
	runCmd.Start()

	// Handle Signals for Cleanup
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	// Wait for the command to finish or signal
	done := make(chan error)
	go func() { done <- runCmd.Wait() }()

	select {
	case <-done:
		// Process finished naturally
	case <-sigChan:
		// User hit Ctrl+C, kill the process group
		syscall.Kill(-runCmd.Process.Pid, syscall.SIGKILL)
	}
}

func runWithWatch(release bool) {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		fmt.Printf("Error creating watcher: %v\n", err)
		return
	}
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

	var currentCmd *exec.Cmd
	isFirstRun := true
	projectName := getProjectName()

	var buildLock sync.Mutex

	restart := func() {
		if !buildLock.TryLock() {
			return
		}
		defer buildLock.Unlock()

		if currentCmd != nil && currentCmd.Process != nil {
			syscall.Kill(-currentCmd.Process.Pid, syscall.SIGKILL)
		}

		if err := RunBlazeBuild(release, isFirstRun); err != nil {
			fmt.Println(err)
			return
		}

		if !isFirstRun {
			fmt.Printf("\n%s\n", blueStyle.Render(" [ Hot Reload ]"))
		}
		isFirstRun = false

		fmt.Print("\033[H\033[2J") // Clear screen
		fmt.Println(blueStyle.Render(fmt.Sprintf("\n  🚀 Blaze Server (%s)\n", projectName)))
		fmt.Printf("  Backend: %s\n", greenStyle.Render("http://localhost:8080"))
		fmt.Printf("  Docs:    %s\n\n", dimStyle.Render("http://localhost:8080/docs"))

		mode := "Debug"
		if release {
			mode = "Release"
		}
		fmt.Printf("  [Build: %s]\n", orangeStyle.Render(mode))
		fmt.Println(dimStyle.Render("  [Hot Reload Active] Watching for changes..."))

		currentCmd = exec.Command("./build/" + projectName)
		currentCmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}
		currentCmd.Stdout = os.Stdout
		currentCmd.Stderr = os.Stderr
		currentCmd.Start()
	}

	restart()

	// Setup Signal Handling
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	// Cleanup on exit
	cleanup := func() {
		if currentCmd != nil && currentCmd.Process != nil {
			syscall.Kill(-currentCmd.Process.Pid, syscall.SIGKILL)
		}
	}
	defer cleanup()

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
					go restart()
				})
			}
		case err, ok := <-watcher.Errors:
			if !ok {
				return
			}
			fmt.Printf("Watcher error: %v\n", err)
		}
	}
}
