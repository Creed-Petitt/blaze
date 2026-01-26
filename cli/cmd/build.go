package cmd

import (
	"fmt"
	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var buildDebug bool

var buildCmd = &cobra.Command{
	Use:   "build",
	Short: "Compile the project without running it",
	Run: func(cmd *cobra.Command, args []string) {
		var (
			titleStyle   = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C"))
			successStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))
		)

		fmt.Println(titleStyle.Render("\n  Starting Blaze Build..."))

		// Build defaults to Release (false for debug = true for release)
		if err := RunBlazeBuild(!buildDebug, true); err != nil {
			fmt.Printf("\nBuild Failed: %v\n", err)
			return
		}

		fmt.Println(successStyle.Render("\n  [+] Build Complete. Binary is in /build\n"))
	},
}

func init() {
	rootCmd.AddCommand(buildCmd)
	buildCmd.Flags().BoolVarP(&buildDebug, "debug", "d", false, "Build in Debug mode instead of Release")
}
