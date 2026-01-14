package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"

	"github.com/spf13/cobra"
)

var dockerCmd = &cobra.Command{
	Use:   "docker",
	Short: "Build a Docker image for the project",
	Long:  `Builds a Docker image using the generated Dockerfile.`,
	Run: func(cmd *cobra.Command, args []string) {
		buildDockerImage()
	},
}

var dockerRunCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the Docker container",
	Long:  `Builds the Docker image and runs it on port 8080.`,
	Run: func(cmd *cobra.Command, args []string) {
		if buildDockerImage() {
			runDockerContainer()
		}
	},
}

func init() {
	rootCmd.AddCommand(dockerCmd)
	dockerCmd.AddCommand(dockerRunCmd)
}

func getProjectName() string {
	cwd, _ := os.Getwd()
	return filepath.Base(cwd)
}

func buildDockerImage() bool {
	name := getProjectName()
	fmt.Printf("Building Docker image: %s\n", name)

	cmd := exec.Command("docker", "build", "-t", name, ".")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	if err := cmd.Run(); err != nil {
		fmt.Printf("Docker build failed: %v\n", err)
		return false
	}
	return true
}

func runDockerContainer() {
	name := getProjectName()
	fmt.Printf("Starting container: %s\n", name)

	// Run on port 8080:8080
	cmd := exec.Command("docker", "run", "-p", "8080:8080", "--rm", name)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin // Forward Ctrl+C

	if err := cmd.Run(); err != nil {
		fmt.Printf("Container stopped: %v\n", err)
	}
}
