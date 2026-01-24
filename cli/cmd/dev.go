package cmd

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"os/signal"
	"strings"
	"sync"
	"syscall"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var devCmd = &cobra.Command{
	Use:   "dev",
	Short: "Run fullstack development environment (C++ + Vite)",
	Long:  `Compiles the C++ backend and starts the Vite frontend server in parallel.`,
	Run: func(cmd *cobra.Command, args []string) {
		runDevEnvironment()
	},
}

func init() {
	rootCmd.AddCommand(devCmd)
}

func runDevEnvironment() {
	var (
		blazeStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true).SetString("[Blaze]")
		viteStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true).SetString("[Vite] ")
		linkStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575")).Underline(true)
	)

	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println("Error: No Blaze project found.")
		return
	}
	if _, err := os.Stat("frontend/package.json"); os.IsNotExist(err) {
		fmt.Println("Error: No frontend found (missing frontend/package.json).")
		return
	}

	if err := RunBlazeBuild(false); err != nil {
		fmt.Printf("\nBuild Failed: %v\n", err)
		return
	}

	projectName := getProjectName()
	backendPath := fmt.Sprintf("./build/%s", projectName)

	backendCmd := exec.Command(backendPath)
	frontendCmd := exec.Command("npm", "run", "dev")
	frontendCmd.Dir = "frontend"

	// Cleanup on Exit
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-c
		if backendCmd.Process != nil {
			backendCmd.Process.Kill()
		}
		if frontendCmd.Process != nil {
			frontendCmd.Process.Kill()
		}
		os.Exit(0)
	}()

	var wg sync.WaitGroup
	wg.Add(2)

	go func() {
		defer wg.Done()
		stdout, _ := backendCmd.StdoutPipe()
		stderr, _ := backendCmd.StderrPipe()
		backendCmd.Start()

		scanner := bufio.NewScanner(stdout)
		for scanner.Scan() {
			line := scanner.Text()
			if line == "" { continue }
			fmt.Printf("%s %s\n", blazeStyle, line)
		}
		
		errScanner := bufio.NewScanner(stderr)
		for errScanner.Scan() {
			fmt.Printf("%s %s\n", blazeStyle, errScanner.Text())
		}
	}()

	go func() {
		defer wg.Done()
		stdout, _ := frontendCmd.StdoutPipe()
		stderr, _ := frontendCmd.StderrPipe()
		frontendCmd.Start()

		scanner := bufio.NewScanner(stdout)
		for scanner.Scan() {
			line := scanner.Text()
			
			// Filter known noise signatures (Empty lines, startup commands, etc)
			if strings.TrimSpace(line) == "" ||
			   strings.Contains(line, "> frontend@") || 
			   strings.Contains(line, "Network: use") || 
			   strings.Contains(line, "press h + enter") ||
			   strings.Contains(line, "ready in") ||
			   strings.Contains(line, "> vite") {
				continue
			}

			// Style the Link
			if strings.Contains(line, "Local:") && strings.Contains(line, "http") {

				parts := strings.Split(line, "http")
				if len(parts) > 1 {
					url := "http" + parts[1]
					// Remove any trailing noise/ansi
					url = strings.Fields(url)[0] 
					fmt.Printf("%s Local:   %s\n", viteStyle, linkStyle.Render(url))
				}
				continue
			}

			// Pass through everything else (Errors, other info)
			fmt.Printf("%s %s\n", viteStyle, line)
		}

		errScanner := bufio.NewScanner(stderr)
		for errScanner.Scan() {
			fmt.Printf("%s %s\n", viteStyle, errScanner.Text())
		}
	}()

	wg.Wait()
}