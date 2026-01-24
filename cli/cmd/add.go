package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"strings"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var (
	titleStyle = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C")) // Red
	checkStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))            // Green
	textStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF"))            // White
	errStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C"))            // Red
)

var addCmd = &cobra.Command{
	Use:   "add [feature]",
	Short: "Add features or drivers to an existing Blaze project",
}

var addFrontendCmd = &cobra.Command{
	Use:   "frontend",
	Short: "Add a Vite-based frontend to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addFrontend()
	},
}

var addMysqlCmd = &cobra.Command{
	Use:   "mysql",
	Short: "Add MySQL driver support to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addDriver("mysql", "libmariadb-dev", "blaze::mysql")
	},
}

var addPostgresCmd = &cobra.Command{
	Use:   "postgres",
	Short: "Add PostgreSQL driver support to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addDriver("postgres", "libpq-dev", "blaze::postgres")
	},
}

func init() {
	rootCmd.AddCommand(addCmd)
	addCmd.AddCommand(addFrontendCmd)
	addCmd.AddCommand(addMysqlCmd)
	addCmd.AddCommand(addPostgresCmd)
}

func addDriver(name, pkgName, libName string) {
	// 1. Verify Project
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println(errStyle.Render("Error: No Blaze project found."))
		return
	}

	fmt.Println(titleStyle.Render(fmt.Sprintf("\n  Adding %s support...", name)))

	// 2. Check System Dependencies (Optional warning)
	cmd := exec.Command("pkg-config", "--exists", strings.ReplaceAll(pkgName, "-dev", ""))
	if err := cmd.Run(); err != nil {
		fmt.Printf("  %s Note: %s might be missing on your system.\n", errStyle.Render("[!]"), pkgName)
		fmt.Printf("    Try: sudo apt install %s\n\n", pkgName)
	}

	// 3. Update CMakeLists.txt
	content, err := os.ReadFile("CMakeLists.txt")
	if err != nil {
		fmt.Println(errStyle.Render("Error reading CMakeLists.txt"))
		return
	}

	cmakeStr := string(content)
	if strings.Contains(cmakeStr, libName) {
		fmt.Println(checkStyle.Render(fmt.Sprintf("  [+] %s is already configured in CMakeLists.txt", name)))
	} else {
		// Insert before the closing parenthesis of target_link_libraries
		newCmake := strings.Replace(cmakeStr, "blaze::framework", "blaze::framework\n    "+libName, 1)
		if newCmake == cmakeStr {
			// Fallback for core-only apps
			newCmake = strings.Replace(cmakeStr, "blaze::core", "blaze::core\n    "+libName, 1)
		}
		
		err = os.WriteFile("CMakeLists.txt", []byte(newCmake), 0644)
		if err != nil {
			fmt.Println(errStyle.Render("Error updating CMakeLists.txt"))
			return
		}
		fmt.Println(checkStyle.Render(fmt.Sprintf("  [+] Added %s to target_link_libraries", libName)))
	}

	// 4. Offer Include Tip
	fmt.Println("")
	fmt.Println(textStyle.Render("  To use this driver, add this to your main.cpp:"))
	fmt.Println(checkStyle.Render(fmt.Sprintf("  #include <blaze/%s.h>", name)))
	fmt.Println("")
}

func addFrontend() {
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

	// Select framework
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

	cmd := exec.Command("npm", "create", "vite@latest", "frontend", "--", "--template", viteTemplate)
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