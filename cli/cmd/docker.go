package cmd

import (
	"fmt"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var (
	dTitleStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true) // Red
	dSuccess    = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))            // Green
	dText       = lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF"))            // White
	dErr        = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C"))            // Red
)

// DockerManager handles Docker operations safely
type DockerManager struct {
	composeCmd []string
}

var dockerMgr = &DockerManager{}

func (m *DockerManager) DetectCompose() {
	// Try 'docker compose' first
	if err := exec.Command("docker", "compose", "version").Run(); err == nil {
		m.composeCmd = []string{"docker", "compose"}
		return
	}
	// Fallback to 'docker-compose'
	if err := exec.Command("docker-compose", "version").Run(); err == nil {
		m.composeCmd = []string{"docker-compose"}
		return
	}
	// Neither found
	m.composeCmd = nil
}

func (m *DockerManager) RunCompose(args ...string) error {
	if len(m.composeCmd) == 0 {
		return fmt.Errorf("docker compose is not installed")
	}
	fullArgs := append(m.composeCmd, args...)
	cmd := exec.Command(fullArgs[0], fullArgs[1:]...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func (m *DockerManager) IsPortFree(port string) bool {
	timeout := time.Second
	conn, err := net.DialTimeout("tcp", net.JoinHostPort("localhost", port), timeout)
	if err != nil {
		return true // Connection failed = Port is free
	}
	if conn != nil {
		conn.Close()
		return false // Connection succeeded = Port is taken
	}
	return true
}

// Commands

var dockerCmd = &cobra.Command{
	Use:   "docker",
	Short: "Docker management for Blaze",
	PersistentPreRun: func(cmd *cobra.Command, args []string) {
		dockerMgr.DetectCompose()
	},
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
		runService("redis", "6379")
	},
}

var psqlCmd = &cobra.Command{
	Use:   "psql",
	Short: "Start a PostgreSQL container",
	Run: func(cmd *cobra.Command, args []string) {
		runService("db", "5432")
	},
}

var mysqlCmd = &cobra.Command{
	Use:   "mysql",
	Short: "Start a MySQL container",
	Run: func(cmd *cobra.Command, args []string) {
		runService("mysql", "3306")
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
	if name == "." || name == "/" {
		name = "blaze-app"
	}
	name = strings.ToLower(strings.ReplaceAll(name, " ", "-"))

	fmt.Println(dTitleStyle.Render(fmt.Sprintf("Building Docker image: %s", name)))
	cmd := exec.Command("docker", "build", "-t", name, ".")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run() == nil
}

func runDockerContainer() {
	if !dockerMgr.IsPortFree("8080") {
		fmt.Println(dErr.Render("Error: Port 8080 is already in use."))
		return
	}
	name := getProjectName()
	fmt.Println(dTitleStyle.Render(fmt.Sprintf("Starting container: %s", name)))
	cmd := exec.Command("docker", "run", "-p", "8080:8080", "--rm", name)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin
	cmd.Run()
}

func runService(service string, port string) {
	fmt.Println(dText.Render(fmt.Sprintf("Starting %s service...", service)))

	if !dockerMgr.IsPortFree(port) {
		fmt.Println(dErr.Render(fmt.Sprintf("Warning: Port %s is already in use.", port)))
		fmt.Println(dText.Render("Container might fail to bind, or you might already have it running."))
	}

	if err := dockerMgr.RunCompose("up", "-d", service); err != nil {
		fmt.Println(dErr.Render(fmt.Sprintf("Error: Failed to start %s.", service)))
	} else {
		fmt.Println(dSuccess.Render(fmt.Sprintf("%s is now running in the background.", service)))
	}
}

func stopCompose() {
	fmt.Println(dTitleStyle.Render("Stopping all Blaze services..."))
	dockerMgr.RunCompose("down")
}

func viewLogs() {
	fmt.Println(dText.Render("Showing Blaze service logs (Ctrl+C to stop)..."))
	dockerMgr.RunCompose("logs", "-f")
}
