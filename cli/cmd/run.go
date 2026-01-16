package cmd

import (
	"fmt"
	"os"
	"os/exec"

	"github.com/spf13/cobra"
)

var runCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the project locally",
	Long:  `Compiles the project using CMake and runs the generated binary.`, 
	Run: func(cmd *cobra.Command, args []string) {
		// Safety check
		if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
			fmt.Println("Error: No Blaze project found in this directory.")
			fmt.Println("Use 'blaze init <name>' to create a new project.")
			return
		}

		if err := RunBlazeBuild(); err != nil {
			fmt.Printf("\n❌ Build Failed: %v\n", err)
			fmt.Println("Check your code or CMakeLists.txt for errors.")
			return
		}

		// Execute Binary
		projectName := getProjectName()
		binaryPath := "./build/" + projectName
		fmt.Printf("\nLaunching %s...\n", projectName)
		
		runCmd := exec.Command(binaryPath, args...)
		runCmd.Stdout = os.Stdout
		runCmd.Stderr = os.Stderr
		runCmd.Run()
	},
}

func init() {
	rootCmd.AddCommand(runCmd)
}