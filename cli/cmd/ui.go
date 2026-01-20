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
	"time"

	"github.com/charmbracelet/bubbles/spinner"
	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

var (
	colorB = lipgloss.Color("214") // Yellow-Orange
	colorL = lipgloss.Color("208") // Pure Orange
	colorA = lipgloss.Color("204") // Orange-Red
	colorZ = lipgloss.Color("202") // Red
	colorE = lipgloss.Color("160") // Deep Red

	styleB = lipgloss.NewStyle().Foreground(colorB).Bold(true)
	styleL = lipgloss.NewStyle().Foreground(colorL).Bold(true)
	styleA = lipgloss.NewStyle().Foreground(colorA).Bold(true)
	styleZ = lipgloss.NewStyle().Foreground(colorZ).Bold(true)
	styleE = lipgloss.NewStyle().Foreground(colorE).Bold(true)

	darkGray     = lipgloss.Color("240")
	unlitStyle   = lipgloss.NewStyle().Foreground(darkGray)
	percentStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("246"))
)

var loadingMessages = []string{
	"Igniting the engine...",
	"Teaching C++ how to be modern...",
	"Fetching Boost (Time for a coffee break?)...",
	"Searching for missing semicolons...",
	"Optimizing the binary for maximum speed...",
	"Convincing the linker to behave...",
	"Converting caffeine into high-performance code...",
	"Blazing through dependencies...",
	"Updating the laws of physics to support async C++...",
	"Are you still reading these?",
	"Watering the server plants...",
	"Asking a rubber duck for architectural advice...",
	"Adjusting the flux capacitor...",
	"Bending the space-time continuum...",
	"Calculating the meaning of life... (it's 42)",
	"Rerouting the orbital lasers...",
	"Counting to infinity... twice.",
	"Generating a sense of urgency...",
	"Look behind you! (Just kidding, keep coding)...",
	"Thinking about what to have for dinner...",
	"Initializing the secret handshake...",
	"Refactoring the universe...",
	"Downloading more RAM...",
	"Everything is fine. Probably.",
}

const (
	logoB1 = `░█████   `
	logoL1 = `░██      `
	logoA1 = `  ░██    `
	logoZ1 = `░███████ `
	logoE1 = `░██████ `

	logoB2 = `░██  ░██ `
	logoL2 = `░██      `
	logoA2 = ` ░██░██  `
	logoZ2 = `   ░███  `
	logoE2 = `░██     `

	logoB3 = `░█████   `
	logoL3 = `░██      `
	logoA3 = `░███████ `
	logoZ3 = `  ░███   `
	logoE3 = `░█████  `

	logoB4 = `░██  ░██ `
	logoL4 = `░██      `
	logoA4 = `░██  ░██ `
	logoZ4 = ` ░███    `
	logoE4 = `░██     `

	logoB5 = `░█████   `
	logoL5 = `░███████ `
	logoA5 = `░██  ░██ `
	logoZ5 = `░███████ `
	logoE5 = `░██████ `
)

type progressMsg float64
type funnyMsg string
type errMsg error
type quitMsg struct{}

type model struct {
	spinner    spinner.Model
	percent    float64
	funnyMsg   string
	err        error
	quitting   bool
}

func initialModel() model {
	s := spinner.New()
	s.Spinner = spinner.Dot
	s.Style = lipgloss.NewStyle().Foreground(colorB)

	return model{
		spinner:    s,
		funnyMsg:   loadingMessages[0],
		percent:    0,
	}
}

func (m model) Init() tea.Cmd {
	return tea.Batch(m.spinner.Tick, tickFunnyMessage())
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		if msg.String() == "ctrl+c" {
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
	if m.err != nil {
		return ""
	}

	done := m.percent >= 1.0

	glowB := m.percent >= 0.05
	glowL := m.percent >= 0.25
	glowA := m.percent >= 0.45
	glowZ := m.percent >= 0.65
	glowE := m.percent >= 0.85

	renderLogo := func(b, l, a, z, e string) string {
		out := ""
		if glowB { out += styleB.Render(b) } else { out += unlitStyle.Render(b) }
		if glowL { out += styleL.Render(l) } else { out += unlitStyle.Render(l) }
		if glowA { out += styleA.Render(a) } else { out += unlitStyle.Render(a) }
		if glowZ { out += styleZ.Render(z) } else { out += unlitStyle.Render(z) }
		if glowE { out += styleE.Render(e) } else { out += unlitStyle.Render(e) }
		return out
	}

	logo := fmt.Sprintf("%s\n%s\n%s\n%s\n%s",
		renderLogo(logoB1, logoL1, logoA1, logoZ1, logoE1),
		renderLogo(logoB2, logoL2, logoA2, logoZ2, logoE2),
		renderLogo(logoB3, logoL3, logoA3, logoZ3, logoE3),
		renderLogo(logoB4, logoL4, logoA4, logoZ4, logoE4),
		renderLogo(logoB5, logoL5, logoA5, logoZ5, logoE5),
	)

	s := fmt.Sprintf("\n%s\n\n", logo) 
	
	if !done {
		pct := int(m.percent * 100)
		s += fmt.Sprintf("%s %s %s\n", m.spinner.View(), percentStyle.Render(fmt.Sprintf("%d%%", pct)), m.funnyMsg)
	} else if !m.quitting {
		s += "\n"
	}

	return s
}

func tickFunnyMessage() tea.Cmd {
	return tea.Tick(time.Second*3, func(t time.Time) tea.Msg {
		return funnyMsg(loadingMessages[rand.Intn(len(loadingMessages))])
	})
}

func RunBlazeBuild() error {
	m := initialModel()
	p := tea.NewProgram(m)

	var buildErr error

	go func() {
		doneChan := make(chan bool)
		go func() {
			ticker := time.NewTicker(time.Millisecond * 100)
			fakePercent := 0.0
			for {
				select {
				case <-doneChan:
					return
				case <-ticker.C:
					if fakePercent < 0.95 {
						fakePercent += 0.002
						p.Send(progressMsg(fakePercent))
					}
				}
			}
		}()

		if err := runCmdWithParsing(p, "cmake", []string{"-B", "build"}, 0, 0.2); err != nil {
			doneChan <- true
			buildErr = err
			p.Send(errMsg(err))
			return
		}

		if err := runCmdWithParsing(p, "cmake", []string{"--build", "build", "--parallel"}, 0.2, 1.0); err != nil {
			doneChan <- true
			buildErr = err
			p.Send(errMsg(err))
			return
		}

		doneChan <- true
		p.Send(progressMsg(1.0))
		time.Sleep(time.Millisecond * 300) 
		p.Send(quitMsg{})
	}()

	if _, err := p.Run(); err != nil {
		return err
	}

	return buildErr
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
		rePercent := regexp.MustCompile(`[\s*(\d+)%]`)
		
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
		return fmt.Errorf("%w\n\n=== Build Output ===\n%s", err, outputLog.String())
	}
	return nil
}