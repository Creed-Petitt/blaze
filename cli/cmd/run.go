package cmd

import (
	"fmt"
	"os"
	"os/exec"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var runRelease bool

var runCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the project locally",
	Run: func(cmd *cobra.Command, args []string) {
		var (
			errStyle     = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C"))
			successStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))
		)

		if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
			fmt.Println(errStyle.Render("Error: No Blaze project found."))
			return
		}

		if err := RunBlazeBuild(runRelease); err != nil {
			fmt.Println(errStyle.Render(fmt.Sprintf("\n Build Failed: %v", err)))
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
	runCmd.Flags().BoolVarP(&runRelease, "release", "r", false, "Run in high-performance Release mode")
}
