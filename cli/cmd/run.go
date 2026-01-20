package cmd

import (
	"fmt"
	"os"
	"os/exec"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var runCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the project locally",
	Long:  `Compiles the project using CMake and runs the generated binary.`, 
	Run: func(cmd *cobra.Command, args []string) {
		var (
			errStyle     = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")) // Red
			successStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575")) // Green
			textStyle    = lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF")) // White
		)

		if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
			fmt.Println(errStyle.Render("Error: No Blaze project found in this directory."))
			fmt.Println(textStyle.Render("Use 'blaze init <name>' to create a new project."))
			return
		}

		if err := RunBlazeBuild(); err != nil {
			fmt.Println(errStyle.Render(fmt.Sprintf("\n Build Failed: %v", err)))
			fmt.Println(textStyle.Render("Check your code or CMakeLists.txt for errors."))
			return
		}

		projectName := getProjectName()
		binaryPath := "./build/" + projectName
		fmt.Println(successStyle.Render(fmt.Sprintf("\nLaunching %s...", projectName)))
		
		runCmd := exec.Command(binaryPath, args...)
		runCmd.Stdout = os.Stdout
		runCmd.Stderr = os.Stderr
		runCmd.Run()
	},
}

func init() {
	rootCmd.AddCommand(runCmd)
}