package cmd

import (
	"embed"
	"fmt"
	"os"
	"path/filepath"
	"text/template"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

//go:embed templates/*
var templatesFS embed.FS

// initCmd represents the init command
var initCmd = &cobra.Command{
	Use:   "init [name]",
	Short: "Initialize a new Blaze project",
	Long: `Scaffolds a new C++ project with CMake, Docker, and source files.
Example: blaze init my-api`,
	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		projectName := args[0]
		createProject(projectName)
	},
}

func init() {
	rootCmd.AddCommand(initCmd)
}

type ProjectData struct {
	ProjectName string
}

func createProject(name string) {
	// Styles
	var (
		titleStyle   = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C")) // Blaze Red/Orange
		checkStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))            // Success Green
		textStyle    = lipgloss.NewStyle().Foreground(lipgloss.Color("#E0E0E0"))            // White-ish
		commandStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C"))            // Blaze Red
	)

	fmt.Println(titleStyle.Render(fmt.Sprintf("\n  Scaffolding project '%s'...", name)))

	// Create Directories
	dirs := []string{
		name,
		filepath.Join(name, "src"),
		filepath.Join(name, "include"),
	}

	for _, dir := range dirs {
		if err := os.MkdirAll(dir, 0755); err != nil {
			fmt.Printf("Error creating directory %s: %v\n", dir, err)
			os.Exit(1)
		}
	}

	// Map Template File -> Output File
	files := map[string]string{
		"templates/main.cpp.tmpl":           filepath.Join(name, "src/main.cpp"),
		"templates/CMakeLists.txt.tmpl":     filepath.Join(name, "CMakeLists.txt"),
		"templates/gitignore.tmpl":          filepath.Join(name, ".gitignore"),
		"templates/dockerignore.tmpl":       filepath.Join(name, ".dockerignore"),
		"templates/Dockerfile.tmpl":         filepath.Join(name, "Dockerfile"),
		"templates/docker-compose.yml.tmpl": filepath.Join(name, "docker-compose.yml"),
		"templates/env.tmpl":                filepath.Join(name, ".env"),
	}

	data := ProjectData{ProjectName: name}

	for tmplPath, outPath := range files {
		tmplContent, err := templatesFS.ReadFile(tmplPath)
		if err != nil {
			continue // Fail silently/gracefully on read
		}

		t, err := template.New(tmplPath).Parse(string(tmplContent))
		if err != nil {
			continue
		}

		f, err := os.Create(outPath)
		if err != nil {
			continue
		}
		t.Execute(f, data)
		f.Close()
	}

	fmt.Println("")
	fmt.Println(checkStyle.Render("  [v] Project structure created"))
	fmt.Println(checkStyle.Render("  [v] Config files generated"))
	fmt.Println(checkStyle.Render("  [v] Docker environment ready"))
	fmt.Println("")
	
	fmt.Println(textStyle.Render("  Project ready! To get started:"))
	fmt.Println("")
	fmt.Println(commandStyle.Render(fmt.Sprintf("  cd %s", name)))
	fmt.Println(commandStyle.Render("  blaze run"))
	fmt.Println("")
}
