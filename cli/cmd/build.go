package cmd

import (
	"fmt"
	"os"
	"os/exec"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var buildDebug bool
var prodMode bool

var buildCmd = &cobra.Command{
	Use:   "build",
	Short: "Compile the project without running it",
	Run: func(cmd *cobra.Command, args []string) {
		var (
			titleStyle   = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#00A2FF"))
			successStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))
			infoStyle    = lipgloss.NewStyle().Foreground(lipgloss.Color("240"))
		)

		if prodMode {
			fmt.Println(titleStyle.Render("\n  Starting Production Bundle..."))

			if _, err := os.Stat("frontend/package.json"); err == nil {
				fmt.Println(infoStyle.Render("  [1/3] Building Frontend..."))

				// npm install
				installCmd := exec.Command("npm", "install")
				installCmd.Dir = "frontend"
				installCmd.Stdout = nil // Silence output unless error
				installCmd.Stderr = os.Stderr
				if err := installCmd.Run(); err != nil {
					fmt.Printf("Frontend install failed: %v\n", err)
					return
				}

				// npm run build
				buildCmd := exec.Command("npm", "run", "build")
				buildCmd.Dir = "frontend"
				buildCmd.Stdout = nil
				buildCmd.Stderr = os.Stderr
				if err := buildCmd.Run(); err != nil {
					fmt.Printf("Frontend build failed: %v\n", err)
					return
				}

				// Move assets
				fmt.Println(infoStyle.Render("  [2/3] Moving Assets to public/..."))
				os.RemoveAll("public")
				os.Mkdir("public", 0755)

				// Copy dist content to public
				// Note: Go doesn't have a simple recursive copy in stdlib, using shell for reliability
				cpCmd := exec.Command("cp", "-r", "frontend/dist/.", "public/")
				if err := cpCmd.Run(); err != nil {
					// Fallback for some systems
					cpCmd = exec.Command("cp", "-r", "frontend/dist", "public/")
					cpCmd.Run()
				}
			} else {
				fmt.Println(infoStyle.Render("  [!] No frontend found, skipping bundle step."))
			}

			fmt.Println(infoStyle.Render("  [3/3] Compiling C++ Binary (Release)..."))
			// Force Release mode for Prod
			if err := RunBlazeBuild(true, true); err != nil {
				fmt.Printf("\nBuild Failed: %v\n", err)
				return
			}
		} else {
			fmt.Println(titleStyle.Render("\n  Starting Blaze Build..."))
			// Standard build
			if err := RunBlazeBuild(!buildDebug, true); err != nil {
				fmt.Printf("\nBuild Failed: %v\n", err)
				return
			}
		}

		fmt.Println(successStyle.Render("\n  [+] Build Complete. Binary is in /build\n"))
	},
}

func init() {
	rootCmd.AddCommand(buildCmd)
	buildCmd.Flags().BoolVarP(&buildDebug, "debug", "d", false, "Build in Debug mode instead of Release")
	buildCmd.Flags().BoolVarP(&prodMode, "prod", "p", false, "Create a production bundle (build frontend + backend)")
}
