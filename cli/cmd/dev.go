package cmd

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"time"

	"github.com/fsnotify/fsnotify"
	"github.com/spf13/cobra"
)

var devCmd = &cobra.Command{
	Use:   "dev",
	Short: "Start the fullstack development environment",
	Run: func(cmd *cobra.Command, args []string) {
		runDev()
	},
}

func init() {
	rootCmd.AddCommand(devCmd)
}

func runDev() {
	if _, err := os.Stat("frontend/package.json"); os.IsNotExist(err) {
		fmt.Println(blueStyle.Render("Error: No frontend found (missing frontend/package.json)."))
		return
	}

	// 1. Initial Build
	if err := RunBlazeBuild(false, true); err != nil {
		fmt.Println(err)
		return
	}

	projectName := getProjectName()
	backendPath := fmt.Sprintf("./build/%s", projectName)

	// Setup Processes
	var backendCmd *exec.Cmd

	frontendCmd := exec.Command("npm", "run", "dev")
	frontendCmd.Dir = "frontend"

	// Pipe BOTH streams to our parser to swallow the noise
	frontendOut, _ := frontendCmd.StdoutPipe()
	frontendErr, _ := frontendCmd.StderrPipe()

	frontendCmd.Start()

	// Parse Frontend URL asynchronously
	parseStream := func(stream io.Reader) {
		scanner := bufio.NewScanner(stream)
		for scanner.Scan() {
			line := scanner.Text()
			if strings.Contains(line, "Local:") && strings.Contains(line, "http") {
				parts := strings.Fields(line)
				for _, p := range parts {
					if strings.HasPrefix(p, "http") {
						// Wait a tiny bit to ensure backend logs print first
						time.Sleep(100 * time.Millisecond)
						fmt.Printf("Frontend: %s\n\n", greenStyle.Render(p))
					}
				}
			}
		}
	}

	go parseStream(frontendOut)
	go parseStream(frontendErr)

	// Watcher for Backend
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

	isFirstRun := true
	var buildLock sync.Mutex

	restartBackend := func() {
		if !buildLock.TryLock() {
			return
		}
		defer buildLock.Unlock()

		if backendCmd != nil && backendCmd.Process != nil {
			backendCmd.Process.Kill()
		}

		if err := RunBlazeBuild(false, isFirstRun); err != nil {
			fmt.Println(err)
			return
		}

		if !isFirstRun {
			fmt.Printf("\n%s\n", blueStyle.Render(" [ Hot Reload ]"))
		}
		isFirstRun = false

		fmt.Printf("Launching %s\n", projectName)
		fmt.Printf("Local:    http://localhost:8080\n")
		fmt.Printf("Docs:     http://localhost:8080/docs\n")
		// Frontend URL will be printed by the goroutine above

		backendCmd = exec.Command(backendPath)
		stdout, _ := backendCmd.StdoutPipe()
		stderr, _ := backendCmd.StderrPipe()
		backendCmd.Start()

		go func() {
			scanner := bufio.NewScanner(stdout)
			for scanner.Scan() {
				fmt.Println(scanner.Text())
			}
		}()
		go func() {
			scanner := bufio.NewScanner(stderr)
			for scanner.Scan() {
				fmt.Println(scanner.Text())
			}
		}()
	}

	restartBackend()

	// Watch Loop with Debounce
	var timer *time.Timer
	for {
		select {
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
