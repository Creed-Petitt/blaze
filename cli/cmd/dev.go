package cmd

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
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
		fmt.Println(orangeStyle.Render("Error: No frontend found (missing frontend/package.json)."))
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

	// Watcher for Backend
	watcher, _ := fsnotify.NewWatcher()
	defer watcher.Close()
	filepath.Walk("src", func(path string, info os.FileInfo, err error) error {
		if info != nil && info.IsDir() { watcher.Add(path) }
		return nil
	})
	filepath.Walk("include", func(path string, info os.FileInfo, err error) error {
		if info != nil && info.IsDir() { watcher.Add(path) }
		return nil
	})

	isFirstRun := true
	restartBackend := func() {
		if backendCmd != nil && backendCmd.Process != nil {
			backendCmd.Process.Kill()
		}
		
		if err := RunBlazeBuild(false, isFirstRun); err != nil {
			fmt.Println(err)
			return
		}

		if !isFirstRun {
			fmt.Printf("\n%s\n", orangeStyle.Render(" [ Hot Reload ]"))
		}
		isFirstRun = false

		fmt.Printf("Launching %s\n", projectName)
		fmt.Printf("Local: http://localhost:8080\n")
		fmt.Printf("Docs:  http://localhost:8080/docs\n\n")

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

	// Frontend Process
	go func() {
		frontendCmd.Stdout = os.Stdout
		frontendCmd.Stderr = os.Stderr
		frontendCmd.Run()
	}()

	// Watch Loop with Debounce
	var timer *time.Timer
	for {
		select {
		case event, ok := <-watcher.Events:
			if !ok { return }
			if event.Op&fsnotify.Write == fsnotify.Write {
				if timer != nil { timer.Stop() }
				timer = time.AfterFunc(100*time.Millisecond, func() {
					restartBackend()
				})
			}
		case <-watcher.Errors:
			return
		}
	}
}
