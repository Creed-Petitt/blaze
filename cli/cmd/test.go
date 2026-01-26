package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"strings"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var testCmd = &cobra.Command{
	Use:   "test",
	Short: "Run the project test suite",
	Run: func(cmd *cobra.Command, args []string) {
		runTests()
	},
}

func init() {
	rootCmd.AddCommand(testCmd)
}

func runTests() {
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println(orangeStyle.Render("Error: No Blaze project found."))
		return
	}

	fmt.Println("\nRunning Blaze Test Suite...")

	m := initialModel(false) 
	p := tea.NewProgram(m)

	go func() {
		// Auto-Configure if build folder is missing
		if _, err := os.Stat("build/CMakeCache.txt"); os.IsNotExist(err) {
			cmakeFlags := []string{"-", "build", "-", "CMAKE_BUILD_TYPE=Debug"}
			if _, err := exec.LookPath("ccache"); err == nil {
				cmakeFlags = append(cmakeFlags, "-", "CMAKE_CXX_COMPILER_LAUNCHER=ccache")
			}
			
			// We use a small range (0.0 - 0.2) for config
			if err := runCmdWithParsing(p, "cmake", cmakeFlags, 0, 0.2); err != nil {
				p.Send(errMsg(err))
				return
			}
		}

		// Build the test target
		err := runCmdWithParsing(p, "cmake", []string{"--build", "build", "--target", "blaze_tests", "--parallel"}, 0.2, 1.0)
		if err != nil {
			p.Send(errMsg(err))
			return
		}
		p.Send(quitMsg{})
	}()

	if _, err := p.Run(); err != nil {
		return
	}

	// Run the binary and capture output
	testRun := exec.Command("./build/blaze_tests")
	output, err := testRun.CombinedOutput()
	outputStr := string(output)

	if err != nil {
		fmt.Println(lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true).Render("\n  [!] Some tests failed:"))
		if len(outputStr) > 0 {
			fmt.Println(outputStr)
		} else {
			fmt.Println("  (No output. The test binary may have failed to start.)")
		}
	} else {
		summary := "All tests passed!"
		lines := strings.Split(outputStr, "\n")
		for _, line := range lines {
			if strings.Contains(line, "passed") && strings.Contains(line, "assertion") {
				summary = strings.TrimSpace(line)
				break
			}
		}
		
		successStyle := lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575")).Bold(true)
		fmt.Printf("\n  %s %s\n\n", successStyle.Render("[+]"), summary)
	}
}