package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"runtime"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var fixMode bool

var doctorCmd = &cobra.Command{
	Use:   "doctor",
	Short: "Check your system for required dependencies",
	Long:  `Scans your environment for compilers, libraries, and tools needed to build Blaze projects.`,
	Run: func(cmd *cobra.Command, args []string) {
		runDoctor(fixMode)
	},
}

func init() {
	rootCmd.AddCommand(doctorCmd)
	doctorCmd.Flags().BoolVarP(&fixMode, "fix", "f", false, "Attempt to automatically install missing dependencies")
}

func runDoctor(fix bool) {
	var (
		whiteStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF"))
		greenCheck   = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575")).SetString("✓")
		redCross     = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).SetString("✗")
		greenText    = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))
		redText      = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C"))
		sectionStyle = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("240")) // Grey for headers
	)

	fmt.Println(whiteStyle.Render("\n  Blaze System Check"))
	fmt.Println("")

	allGood := true
	missingPkgs := []string{}

	checkCommand := func(name string, display string) bool {
		path, err := exec.LookPath(name)
		if err == nil {
			fmt.Printf("  %s %s (%s)\n", greenCheck, display, path)
			return true
		}
		fmt.Printf("  %s %s (Not found)\n", redCross, display)
		return false
	}

	checkLib := func(pkgName, display, installName string) bool {
		cmd := exec.Command("pkg-config", "--exists", pkgName)
		if err := cmd.Run(); err == nil {
			fmt.Printf("  %s %s\n", greenCheck, display)
			return true
		}
		fmt.Printf("  %s %s (Not found)\n", redCross, display)
		if installName != "" {
			missingPkgs = append(missingPkgs, installName)
		}
		return false
	}

	fmt.Println(sectionStyle.Render("  Core Tools"))
	if !checkCommand("g++", "G++ Compiler") && !checkCommand("clang++", "Clang++ Compiler") {
		allGood = false
	}
	if !checkCommand("cmake", "CMake Build System") { allGood = false }
	checkCommand("docker", "Docker Engine") 

	fmt.Println("")
	fmt.Println(sectionStyle.Render("  Libraries"))

	if runtime.GOOS == "linux" {
		if !checkLib("openssl", "OpenSSL Dev Libs", "libssl-dev") { allGood = false }
		checkLib("libpq", "PostgreSQL Libs", "libpq-dev")
		checkLib("libmariadb", "MariaDB Connector", "libmariadb-dev")
	} else if runtime.GOOS == "darwin" {
		if !checkLib("openssl", "OpenSSL Dev Libs", "openssl") { allGood = false }
		checkLib("libpq", "PostgreSQL Libs", "libpq")
	}

	if fix && len(missingPkgs) > 0 {
		fmt.Println(whiteStyle.Render("\n  Attempting to fix missing dependencies..."))
		var installCmd *exec.Cmd
		if runtime.GOOS == "linux" {
			args := append([]string{"apt-get", "install", "-y"}, missingPkgs...)
			installCmd = exec.Command("sudo", args...)
		} else if runtime.GOOS == "darwin" {
			args := append([]string{"install"}, missingPkgs...)
			installCmd = exec.Command("brew", args...)
		}

		if installCmd != nil {
			installCmd.Stdout = os.Stdout
			installCmd.Stderr = os.Stderr
			if err := installCmd.Run(); err == nil {
				fmt.Println(greenText.Render("\n  Successfully installed dependencies. Run doctor again!"))
				return
			}
		}
	}

	fmt.Println("")
	if allGood {
		fmt.Println(greenText.Render("  System Ready. You are good to go!"))
	} else {
		fmt.Println(redText.Render("  Some requirements are missing."))
		if !fix {
			fmt.Println(whiteStyle.Render("  Tip: Run 'blaze doctor --fix' to auto-install missing libs."))
		}
	}
	fmt.Println("")
}