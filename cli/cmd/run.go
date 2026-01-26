package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"sync"
	"time"

	"github.com/fsnotify/fsnotify"
	"github.com/spf13/cobra"
)

var watchMode bool

var runCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the Blaze project",
	Run: func(cmd *cobra.Command, args []string) {
		release, _ := cmd.Flags().GetBool("release")
		watch, _ := cmd.Flags().GetBool("watch")

		if watch {
			runWithWatch(release)
		} else {
			runStatic(release)
		}
	},
}

func init() {
	rootCmd.AddCommand(runCmd)
	runCmd.Flags().BoolVarP(&watchMode, "watch", "w", false, "Watch for changes and hot-reload")
	runCmd.Flags().BoolP("release", "r", false, "Build in Release mode")
}

func runStatic(release bool) {
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println(orangeStyle.Render("Error: No Blaze project found."))
		return
	}

	if err := RunBlazeBuild(release, true); err != nil {
		fmt.Println(err)
		return
	}

	projectName := getProjectName()
	fmt.Printf("\nLaunching %s\n", projectName)
	fmt.Printf("Local: http://localhost:8080\n")
	fmt.Printf("Docs:  http://localhost:8080/docs\n\n")
	
	runCmd := exec.Command("./build/" + projectName)
	runCmd.Stdout = os.Stdout
	runCmd.Stderr = os.Stderr
	runCmd.Run()
}

func runWithWatch(release bool) {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		fmt.Printf("Error creating watcher: %v\n", err)
		return
	}
	defer watcher.Close()

	filepath.Walk("src", func(path string, info os.FileInfo, err error) error {
		if info != nil && info.IsDir() { watcher.Add(path) }
		return nil
	})
	filepath.Walk("include", func(path string, info os.FileInfo, err error) error {
		if info != nil && info.IsDir() { watcher.Add(path) }
		return nil
	})

	var currentCmd *exec.Cmd
	isFirstRun := true
	projectName := getProjectName()
	
	var buildLock sync.Mutex

	restart := func() {
		if !buildLock.TryLock() { return }
		defer buildLock.Unlock()

		if currentCmd != nil && currentCmd.Process != nil {
			currentCmd.Process.Kill()
		}
		
		if err := RunBlazeBuild(release, isFirstRun); err != nil {
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

		currentCmd = exec.Command("./build/" + projectName)
		currentCmd.Stdout = os.Stdout
		currentCmd.Stderr = os.Stderr
		currentCmd.Start()
	}

	restart()

	var timer *time.Timer
	for {
		select {
		case event, ok := <-watcher.Events:
			if !ok { return }
			if event.Op&fsnotify.Write == fsnotify.Write {
				if timer != nil { timer.Stop() }
				timer = time.AfterFunc(100*time.Millisecond, func() {
					go restart()
				})
			}
		case err, ok := <-watcher.Errors:
			if !ok { return }
			fmt.Printf("Watcher error: %v\n", err)
		}
	}
}
