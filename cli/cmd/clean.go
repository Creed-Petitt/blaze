package cmd

import (
	"fmt"
	"os"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var cleanCmd = &cobra.Command{
	Use:   "clean",
	Short: "Remove build artifacts and cache",
	Run: func(cmd *cobra.Command, args []string) {
		var (
			titleStyle = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C"))
			checkStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))
		)

		fmt.Println(titleStyle.Render("\n  Cleaning project..."))

		// Remove build directory
		if _, err := os.Stat("build"); err == nil {
			err := os.RemoveAll("build")
			if err != nil {
				fmt.Printf("  Error removing build directory: %v\n", err)
				return
			}
			fmt.Println(checkStyle.Render("  [+] Removed /build directory"))
		} else {
			fmt.Println("  [ ] Nothing to clean (no build directory found)")
		}

		fmt.Println(checkStyle.Render("  [+] Project is clean.\n"))
	},
}

func init() {
	rootCmd.AddCommand(cleanCmd)
}
