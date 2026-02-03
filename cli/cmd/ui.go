package cmd

import (
	"bufio"
	"fmt"
	"io"
	"math/rand"
	"os/exec"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/charmbracelet/bubbles/spinner"
	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

var (
	uiLock       sync.Mutex
	colorB       = lipgloss.Color("#00A2FF") // Blaze Blue
	styleB       = lipgloss.NewStyle().Foreground(colorB).Bold(true)
	blueStyle    = lipgloss.NewStyle().Foreground(colorB)
	orangeStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("202")).Bold(true)
	greenStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("46")).Bold(true)
	whiteStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("15")).Bold(true)
	percentStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("246"))
	dimStyle     = lipgloss.NewStyle().Foreground(lipgloss.Color("240"))
)

const banner = `
   (   )\ )    (      ( /(       
 ( )\ (()/(    )\     )\()) (    
 )((_) /(_))((((_)(  ((_)\  )\   
((_)_ (_))   )\_ \ )  _((_)((_)  
 | _ )| |    (_) \(_)|_  / | __| 
 | _ \| |__   / _ \   / /  | _|  
 |___/|____| /_/ \_\ /___| |___| 
`

var loadingMessages = []string{
	"Igniting the engine...",
	"Optimizing binaries...",
	"Linker is thinking...",
	"Making C++ modern...",
}

type progressMsg float64
type funnyMsg string
type errMsg error
type quitMsg struct{}

type taskModel struct {
	spinner spinner.Model
	message string
	err     error
	done    bool
}

func (m taskModel) Init() tea.Cmd { return m.spinner.Tick }
func (m taskModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		if msg.String() == "ctrl+c" {
			return m, tea.Quit
		}
	case spinner.TickMsg:
		var cmd tea.Cmd
		m.spinner, cmd = m.spinner.Update(msg)
		return m, cmd
	case quitMsg:
		m.done = true
		return m, tea.Quit
	case errMsg:
		m.err = msg
		return m, tea.Quit
	}
	return m, nil
}
func (m taskModel) View() string {
	if m.done {
		return greenStyle.Render("  ✓ ") + whiteStyle.Render(m.message) + "\n"
	}
	if m.err != nil {
		return orangeStyle.Render("  ✗ ") + whiteStyle.Render(m.message) + "\n"
	}
	return fmt.Sprintf("  %s %s", m.spinner.View(), whiteStyle.Render(m.message))
}

func RunTaskWithSpinner(message string, task func() error) error {
	s := spinner.New()
	s.Spinner = spinner.Dot
	s.Style = lipgloss.NewStyle().Foreground(colorB)

	m := taskModel{
		spinner: s,
		message: message,
	}

	p := tea.NewProgram(m)

	var taskErr error
	go func() {
		taskErr = task()
		if taskErr != nil {
			p.Send(errMsg(taskErr))
		} else {
			p.Send(quitMsg{})
		}
	}()

	if _, err := p.Run(); err != nil {
		return err
	}
	return taskErr
}

type model struct {
	spinner   spinner.Model
	percent   float64
	funnyMsg  string
	err       error
	quitting  bool
	cancelled bool
	showLogo  bool
}

func initialModel(showLogo bool) model {
	s := spinner.New()
	s.Spinner = spinner.Line
	s.Style = lipgloss.NewStyle().Foreground(colorB)

	return model{
		spinner:   s,
		funnyMsg:  loadingMessages[0],
		percent:   0,
		showLogo:  showLogo,
		cancelled: false,
	}
}

func (m model) Init() tea.Cmd {
	return tea.Batch(m.spinner.Tick, tickFunnyMessage())
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		if msg.String() == "ctrl+c" {
			m.cancelled = true
			return m, tea.Quit
		}
	case spinner.TickMsg:
		var cmd tea.Cmd
		m.spinner, cmd = m.spinner.Update(msg)
		return m, cmd
	case progressMsg:
		m.percent = float64(msg)
		return m, nil
	case funnyMsg:
		m.funnyMsg = string(msg)
		return m, tickFunnyMessage()
	case errMsg:
		m.err = msg
		return m, tea.Quit
	case quitMsg:
		m.quitting = true
		return m, tea.Quit
	}
	return m, nil
}

func (m model) View() string {
	if m.quitting || m.err != nil {
		return ""
	}

	var s string
	if m.showLogo {
		s += whiteStyle.Render(banner) + "\n"
	}

	width := 30
	full := int(m.percent * float64(width))
	if full > width {
		full = width
	}
	bar := styleB.Render(strings.Repeat("━", full)) + dimStyle.Render(strings.Repeat("━", width-full))

	pct := int(m.percent * 100)
	if pct > 100 {
		pct = 100
	}

	s += fmt.Sprintf("  %s %s %s %s\n",
		m.spinner.View(),
		bar,
		percentStyle.Render(fmt.Sprintf("%3d%%", pct)),
		m.funnyMsg,
	)
	return s
}

func tickFunnyMessage() tea.Cmd {
	return tea.Tick(time.Second*3, func(t time.Time) tea.Msg {
		return funnyMsg(loadingMessages[rand.Intn(len(loadingMessages))])
	})
}

func RunBlazeBuild(release bool, showLogo bool) error {
	if !uiLock.TryLock() {
		// If UI is busy (e.g. hot reload overlap), just run quietly
		// But we still want to block until it's done
		return runBuildQuietly(release)
	}
	defer uiLock.Unlock()

	m := initialModel(showLogo)
	p := tea.NewProgram(m)

	buildMode := "Debug"
	if release {
		buildMode = "Release"
	}

	var buildErr error
	doneChan := make(chan bool)

	go func() {
		// Auto-Detect ccache
		cmakeFlags := []string{"-B", "build", fmt.Sprintf("-DCMAKE_BUILD_TYPE=%s", buildMode), "."}
		if _, err := exec.LookPath("ccache"); err == nil {
			cmakeFlags = append(cmakeFlags, "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache")
		}

		// Initial config - slow progress
		p.Send(progressMsg(0.05))
		if err := runCmdWithParsing(p, "cmake", cmakeFlags, 0.05, 0.20); err != nil {
			close(doneChan)
			buildErr = err
			p.Send(errMsg(err))
			return
		}

		// Build - real progress
		if err := runCmdWithParsing(p, "cmake", []string{"--build", "build", "--parallel"}, 0.20, 1.0); err != nil {
			close(doneChan)
			buildErr = err
			p.Send(errMsg(err))
			return
		}

		close(doneChan)
		p.Send(progressMsg(1.0))
		time.Sleep(time.Millisecond * 100) // Ensure 100% is seen
		p.Send(quitMsg{})
	}()

	if finalModel, err := p.Run(); err != nil {
		return err
	} else if m, ok := finalModel.(model); ok && m.cancelled {
		return fmt.Errorf("build cancelled by user")
	}

	return buildErr
}

func runBuildQuietly(release bool) error {
	buildMode := "Debug"
	if release {
		buildMode = "Release"
	}
	// Re-configure just in case
	exec.Command("cmake", "-B", "build", fmt.Sprintf("-DCMAKE_BUILD_TYPE=%s", buildMode), ".").Run()

	cmd := exec.Command("cmake", "--build", "build", "--parallel")
	if output, err := cmd.CombinedOutput(); err != nil {
		return BeautifyError(string(output))
	}
	return nil
}

func runCmdWithParsing(p *tea.Program, command string, args []string, startRange, endRange float64) error {
	cmd := exec.Command(command, args...)
	pr, pw := io.Pipe()
	cmd.Stdout = pw
	cmd.Stderr = pw
	var outputLog strings.Builder

	if err := cmd.Start(); err != nil {
		return err
	}

	go func() {
		scanner := bufio.NewScanner(pr)
		rePercent := regexp.MustCompile(`\[\s*(\d+)%\]`)
		for scanner.Scan() {
			line := scanner.Text()
			outputLog.WriteString(line + "\n")
			if matches := rePercent.FindStringSubmatch(line); len(matches) > 1 {
				val, _ := strconv.Atoi(matches[1])
				progress := startRange + (float64(val)/100.0)*(endRange-startRange)
				p.Send(progressMsg(progress))
			}
		}
	}()

	err := cmd.Wait()
	pw.Close()

	if err != nil {
		return BeautifyError(outputLog.String())
	}
	return nil
}

func BeautifyError(raw string) error {
	headerStyle := lipgloss.NewStyle().Background(colorB).Foreground(lipgloss.Color("#FFFFFF")).Padding(0, 1).Bold(true)
	errStyle := lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true)

	out := "\n" + headerStyle.Render(" COMPILER ERROR ") + "\n\n"

	lines := strings.Split(raw, "\n")
	var errorLines []string
	for _, line := range lines {
		trimmed := strings.TrimSpace(line)
		if trimmed == "" {
			continue
		}
		if strings.Contains(line, "error:") || strings.Contains(line, "CMake Error") {
			errorLines = append(errorLines, "  "+trimmed)
		}
	}

	if len(errorLines) == 0 {
		out += errStyle.Render("  [!] Build failed:") + "\n"
		// Capture last 15 lines for context (Linker errors, CMake config issues)
		start := len(lines) - 15
		if start < 0 {
			start = 0
		}
		for i := start; i < len(lines); i++ {
			if t := strings.TrimSpace(lines[i]); t != "" {
				out += "  " + t + "\n"
			}
		}
	} else {
		out += errStyle.Render("  [!] Error Details:") + "\n"
		limit := 10
		if len(errorLines) < limit {
			limit = len(errorLines)
		}
		for i := 0; i < limit; i++ {
			out += errorLines[i] + "\n"
		}
	}

	return fmt.Errorf("%s", out)
}
