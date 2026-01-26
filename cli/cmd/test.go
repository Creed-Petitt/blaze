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

	// 1. Build the test target
	m := initialModel(false) 
	p := tea.NewProgram(m)

	go func() {
		err := runCmdWithParsing(p, "cmake", []string{"--build", "build", "--target", "blaze_tests", "--parallel"}, 0, 1.0)
		if err != nil {
			p.Send(errMsg(err))
			return
		}
		p.Send(quitMsg{})
	}()

	if _, err := p.Run(); err != nil {
		fmt.Printf("Build failed: %v\n", err)
		return
	}

	// 2. Run the binary and capture output
	testRun := exec.Command("./build/blaze_tests")
	output, err := testRun.CombinedOutput()
	outputStr := string(output)

	if err != nil {
		// Failure: Show the raw details so the user can debug
		fmt.Println(lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true).Render("\n  [!] Some tests failed:"))
		fmt.Println(outputStr)
	} else {
		// Success: Extract the numbers from Catch2 output
		// Format: "All tests passed (1 assertion in 1 test case)"
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
