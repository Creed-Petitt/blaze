package cmd

import (
	"embed"
	"fmt"
	"os"
	"path/filepath"
	"text/template"

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
	fmt.Printf("Initializing Blaze project: %s\n", name)

	// Create Directories
	dirs := []string{
		name,
		filepath.Join(name, "src"),
		filepath.Join(name, "include"), // Empty for now, but good practice
	}

	for _, dir := range dirs {
		if err := os.MkdirAll(dir, 0755); err != nil {
			fmt.Printf("Error creating directory %s: %v\n", dir, err)
			os.Exit(1)
		}
	}

	// Map Template File -> Output File
	files := map[string]string{
		"templates/main.cpp.tmpl":       filepath.Join(name, "src/main.cpp"),
		"templates/CMakeLists.txt.tmpl": filepath.Join(name, "CMakeLists.txt"),
		"templates/gitignore.tmpl":      filepath.Join(name, ".gitignore"),
		"templates/Dockerfile.tmpl":     filepath.Join(name, "Dockerfile"),
	}

	data := ProjectData{ProjectName: name}

	for tmplPath, outPath := range files {
		// Read template from embedded FS
		tmplContent, err := templatesFS.ReadFile(tmplPath)
		if err != nil {
			fmt.Printf("Error reading template %s: %v\n", tmplPath, err)
			continue
		}

		// Parse template
		t, err := template.New(tmplPath).Parse(string(tmplContent))
		if err != nil {
			fmt.Printf("Error parsing template %s: %v\n", tmplPath, err)
			continue
		}

		// Execute template logic in a closure to safely use defer for Close()
		err = func() error {
			f, err := os.Create(outPath)
			if err != nil {
				return err
			}

			// Safe defer that handles the error explicitly
			defer func() {
				if closeErr := f.Close(); closeErr != nil {
					fmt.Printf("Warning: failed to close file %s: %v\n", outPath, closeErr)
				}
			}()

			return t.Execute(f, data)
		}()

		if err != nil {
			fmt.Printf("Error generating %s: %v\n", outPath, err)
			continue
		}

		fmt.Printf("Created %s\n", outPath)
	}

	fmt.Printf("\nProject ready! Run:\n  cd %s\n  blaze run\n", name)
}
