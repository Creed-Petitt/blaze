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
	Short: "Docker management for Blaze",
}

var dockerBuildCmd = &cobra.Command{
	Use:   "build",
	Short: "Build the app Docker image",
	Run: func(cmd *cobra.Command, args []string) {
		buildDockerImage()
	},
}

var dockerRunCmd = &cobra.Command{
	Use:   "run",
	Short: "Build and run the app container",
	Run: func(cmd *cobra.Command, args []string) {
		if buildDockerImage() {
			runDockerContainer()
		}
	},
}

var redisCmd = &cobra.Command{
	Use:   "redis",
	Short: "Start a Redis container",
	Run: func(cmd *cobra.Command, args []string) {
		runComposeService("redis")
	},
}

var psqlCmd = &cobra.Command{
	Use:   "psql",
	Short: "Start a PostgreSQL container",
	Run: func(cmd *cobra.Command, args []string) {
		runComposeService("db")
	},
}

var mysqlCmd = &cobra.Command{
	Use:   "mysql",
	Short: "Start a MySQL container",
	Run: func(cmd *cobra.Command, args []string) {
		runComposeService("mysql")
	},
}

var stopCmd = &cobra.Command{
	Use:   "stop",
	Short: "Stop all background containers",
	Run: func(cmd *cobra.Command, args []string) {
		stopCompose()
	},
}

var logsCmd = &cobra.Command{
	Use:   "logs",
	Short: "View logs from background containers",
	Run: func(cmd *cobra.Command, args []string) {
		viewLogs()
	},
}

func init() {
	rootCmd.AddCommand(dockerCmd)
	dockerCmd.AddCommand(dockerBuildCmd)
	dockerCmd.AddCommand(dockerRunCmd)
	dockerCmd.AddCommand(redisCmd)
	dockerCmd.AddCommand(psqlCmd)
	dockerCmd.AddCommand(mysqlCmd)
	dockerCmd.AddCommand(stopCmd)
	dockerCmd.AddCommand(logsCmd)
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
	return cmd.Run() == nil
}

func runDockerContainer() {
	name := getProjectName()
	fmt.Printf("Starting container: %s\n", name)
	cmd := exec.Command("docker", "run", "-p", "8080:8080", "--rm", name)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin
	cmd.Run()
}

func runComposeService(service string) {
	fmt.Printf("Starting %s service...\n", service)

	// Try modern "docker compose"
	cmd := exec.Command("docker", "compose", "up", "-d", service)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	
	if err := cmd.Run(); err != nil {
		// Fallback to old "docker-compose"
		fmt.Println("Modern 'docker compose' failed, trying legacy 'docker-compose'...")
		legacyCmd := exec.Command("docker-compose", "up", "-d", service)
		legacyCmd.Stdout = os.Stdout
		legacyCmd.Stderr = os.Stderr
		
		if err := legacyCmd.Run(); err != nil {
			fmt.Printf("Error: Failed to start %s. Please ensure Docker Compose is installed.\n", service)
		} else {
			fmt.Printf("%s (legacy) is now running in the background.\n", service)
		}
	} else {
		fmt.Printf("%s is now running in the background.\n", service)
	}
}

func stopCompose() {
	fmt.Println("Stopping all Blaze services...")
	cmd := exec.Command("docker", "compose", "down")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	
	if err := cmd.Run(); err != nil {
		legacyCmd := exec.Command("docker-compose", "down")
		legacyCmd.Stdout = os.Stdout
		legacyCmd.Stderr = os.Stderr
		legacyCmd.Run()
	}
}

func viewLogs() {
	fmt.Println("Showing Blaze service logs (Ctrl+C to stop)...")
	cmd := exec.Command("docker", "compose", "logs", "-f")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin
	cmd.Run()
}