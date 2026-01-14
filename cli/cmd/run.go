package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"

	"github.com/spf13/cobra"
)

var runCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the project",
	Long:  `Compiles the C++ project using CMake and executes the resulting binary.`, 
	Run: func(cmd *cobra.Command, args []string) {
		runProject()
	},
}

func init() {
	rootCmd.AddCommand(runCmd)
}

func runProject() {
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println("Error: CMakeLists.txt not found. Are you in the project root?")
		os.Exit(1)
	}

	fmt.Println("Building project...")

	// Configure
	config := exec.Command("cmake", "-B", "build")
	config.Stdout = os.Stdout
	config.Stderr = os.Stderr
	if err := config.Run(); err != nil {
		fmt.Printf("Configuration failed: %v\n", err)
		os.Exit(1)
	}

	// Build
	build := exec.Command("cmake", "--build", "build")
	build.Stdout = os.Stdout
	build.Stderr = os.Stderr
	if err := build.Run(); err != nil {
		fmt.Printf("Build failed: %v\n", err)
		os.Exit(1)
	}

	// Identify and Run
	cwd, _ := os.Getwd()
	projectName := filepath.Base(cwd)
	exePath := filepath.Join("build", projectName)
	if runtime.GOOS == "windows" {
		exePath += ".exe"
	}

	fmt.Printf("Starting %s...\n\n", projectName)

	server := exec.Command(exePath)
	server.Stdout = os.Stdout
	server.Stderr = os.Stderr
	server.Stdin = os.Stdin

	if err := server.Run(); err != nil {
		fmt.Printf("Application exited: %v\n", err)
		os.Exit(1)
	}
}
