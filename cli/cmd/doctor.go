package cmd

import (
	"fmt"
	"os/exec"
	"runtime"

	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var doctorCmd = &cobra.Command{
	Use:   "doctor",
	Short: "Check your system for required dependencies",
	Long:  `Scans your environment for compilers, libraries, and tools needed to build Blaze projects.`,
	Run: func(cmd *cobra.Command, args []string) {
		runDoctor()
	},
}

func init() {
	rootCmd.AddCommand(doctorCmd)
}

func runDoctor() {
	var (
		titleStyle   = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C")) // Blaze Red
		sectionStyle = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FFFFFF")) // White
		successStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))            // Green
		failStyle    = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C"))            // Red
	)

	fmt.Println(titleStyle.Render("\n  Blaze System Check"))
	fmt.Println(sectionStyle.Render("  ------------------ "))

	allGood := true

	checkCommand := func(name string, display string) bool {
		path, err := exec.LookPath(name)
		if err == nil {
			fmt.Printf("  %s %s (%s)\n", successStyle.Render("[+]"), sectionStyle.Render(display), path)
			return true
		}
		fmt.Printf("  %s %s (Not found)\n", failStyle.Render("[!]"), sectionStyle.Render(display))
		return false
	}

	// Helper to check libraries (using pkg-config or basic header check logic could go here)
	// For now, we will assume if the user has the compiler, they might have libs, 
	// but a robust check usually requires pkg-config.
	checkLib := func(pkgName string, display string) bool {
		// Try pkg-config first
		cmd := exec.Command("pkg-config", "--exists", pkgName)
		if err := cmd.Run(); err == nil {
			fmt.Printf("  %s %s (Found via pkg-config)\n", successStyle.Render("[+]"), sectionStyle.Render(display))
			return true
		}
		// Fallback: This is hard to detect reliably across all OSs without compiling.
		// We will mark it as Warning/Fail
		fmt.Printf("  %s %s (Not found)\n", failStyle.Render("[!]"), sectionStyle.Render(display))
		return false
	}

	fmt.Println("")
	fmt.Println(sectionStyle.Render("  Core Tools"))

	hasGcc := checkCommand("g++", "G++ Compiler")
	hasClang := checkCommand("clang++", "Clang++ Compiler")
	if !hasGcc && !hasClang {
		allGood = false
		fmt.Println(sectionStyle.Render("      -> You need a C++20 compiler (g++ or clang++)"))
	}

	if !checkCommand("cmake", "CMake Build System") {
		allGood = false
		fmt.Println(sectionStyle.Render("      -> Required to build projects"))
	}

	checkCommand("docker", "Docker Engine") 

	fmt.Println("")
	fmt.Println(sectionStyle.Render("  Libraries"))

	// Note: specifically for the development headers
	if !checkLib("openssl", "OpenSSL Dev Libs") {
		allGood = false
		if runtime.GOOS == "linux" {
			fmt.Println(sectionStyle.Render("      -> Try: sudo apt install libssl-dev"))
		} else if runtime.GOOS == "darwin" {
			fmt.Println(sectionStyle.Render("      -> Try: brew install openssl"))
		}
	}

	if !checkLib("libpq", "PostgreSQL Libs") {
		// Not strictly fatal if they don't use Postgres, but we warn
		fmt.Println(sectionStyle.Render("      -> Optional. Needed for Postgres driver."))
		if runtime.GOOS == "linux" {
			fmt.Println(sectionStyle.Render("      -> Try: sudo apt install libpq-dev"))
		}
	}
	
	if !checkLib("libmariadb", "MariaDB Connector (Async)") {
		fmt.Println(sectionStyle.Render("      -> Required for MySQL async driver."))
		if runtime.GOOS == "linux" {
			fmt.Println(sectionStyle.Render("      -> Try: sudo apt install libmariadb-dev"))
		} else if runtime.GOOS == "darwin" {
			fmt.Println(sectionStyle.Render("      -> Try: brew install mariadb-connector-c"))
		}
	}

	fmt.Println("")
	if allGood {
		fmt.Println(successStyle.Render("  [+] System Ready. You are good to go!"))
	} else {
		fmt.Println(failStyle.Render("  [!] Some requirements are missing."))
		fmt.Println(sectionStyle.Render("      Run the suggested commands to fix them."))
	}
	fmt.Println("")
}
