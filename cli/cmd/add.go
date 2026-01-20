package cmd

import (
	"fmt"
	"os"
	"os/exec"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var addCmd = &cobra.Command{
	Use:   "add [feature]",
	Short: "Add features to an existing Blaze project",
}

var addFrontendCmd = &cobra.Command{
	Use:   "frontend",
	Short: "Add a Vite-based frontend to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addFrontend()
	},
}

func init() {
	rootCmd.AddCommand(addCmd)
	addCmd.AddCommand(addFrontendCmd)
}

func addFrontend() {
	var (
		titleStyle = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C")) // Red
		checkStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))            // Green
		textStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF"))            // White
		errStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C"))            // Red
	)

	// Check for existing project
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println(errStyle.Render("Error: No Blaze project found. (CMakeLists.txt missing)"))
		return
	}

	// Check if frontend already exists
	if _, err := os.Stat("frontend"); err == nil {
		fmt.Println(errStyle.Render("Error: A 'frontend' directory already exists."))
		return
	}

	checkNpm()

	// Select framework (reuse existing logic from init.go)
	framework := selectFrontend()
	if framework == "" {
		fmt.Println("Selection cancelled.")
		return
	}

	fmt.Println(titleStyle.Render(fmt.Sprintf("\n  Adding %s frontend...", framework)))

	viteTemplate := "vanilla-ts"
	switch framework {
	case "React":
		viteTemplate = "react-ts"
	case "Vue":
		viteTemplate = "vue-ts"
	case "Svelte":
		viteTemplate = "svelte-ts"
	case "Solid":
		viteTemplate = "solid-ts"
	}

	// Create 'frontend' dir directly here
	// npm create vite @latest frontend ...
	cmd := exec.Command("npm", "create", "vite@latest", "frontend", "--", "--template", viteTemplate)
	
	// Suppress output
	cmd.Stdout = nil
	cmd.Stderr = os.Stderr

	if err := cmd.Run(); err != nil {
		fmt.Println(errStyle.Render(fmt.Sprintf("  Error creating frontend: %v", err)))
		return
	}

	fmt.Println(checkStyle.Render(fmt.Sprintf("  [+] Frontend (%s) added in /frontend", framework)))
	fmt.Println("")
	fmt.Println(textStyle.Render("  To get started:"))
	fmt.Println(textStyle.Render("  cd frontend && npm install && cd .."))
	fmt.Println(textStyle.Render("  blaze dev"))
	fmt.Println("")
}
