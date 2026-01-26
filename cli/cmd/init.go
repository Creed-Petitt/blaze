package cmd

import (
	"embed"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"text/template"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

//go:embed templates/*
var templatesFS embed.FS

var fullstackMode bool

var initCmd = &cobra.Command{
	Use:   "init [name]",
	Short: "Initialize a new Blaze project",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		projectName := args[0]

		// Sanitize project name
		if strings.Contains(projectName, "..") || strings.Contains(projectName, "/") || strings.Contains(projectName, "\\") {
			fmt.Println(orangeStyle.Render("\n  [!] Error: Project name cannot contain path separators or '..'"))
			os.Exit(1)
		}

		features := selectFeatures()

		var frontend string
		if fullstackMode {
			checkNpm()
			frontend = selectFrontend()
			if frontend == "" {
				os.Exit(0)
			}
		}
		createProject(projectName, frontend, features)
	},
}

func init() {
	rootCmd.AddCommand(initCmd)
	initCmd.Flags().BoolVarP(&fullstackMode, "fullstack", "f", false, "Initialize with a Frontend")
}

type ProjectData struct {
	ProjectName string
}

// --- Backend Feature Selection ---

type featureItem struct {
	title    string
	selected bool
}

type featureSelectionModel struct {
	choices []featureItem
	cursor  int
	done    bool
}

func initialFeatureModel() featureSelectionModel {
	return featureSelectionModel{
		choices: []featureItem{
			{title: "PostgreSQL", selected: false},
			{title: "MySQL", selected: false},
			{title: "Catch2 Testing", selected: false},
		},
	}
}

func (m featureSelectionModel) Init() tea.Cmd { return nil }

func (m featureSelectionModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "ctrl+c", "q", "esc":
			return m, tea.Quit
		case "up", "k":
			if m.cursor > 0 {
				m.cursor--
			}
		case "down", "j":
			if m.cursor < len(m.choices) {
				m.cursor++
			}
		case "enter", " ", "x":
			if m.cursor == len(m.choices) {
				m.done = true
				return m, tea.Quit
			}
			m.choices[m.cursor].selected = !m.choices[m.cursor].selected
		}
	}
	return m, nil
}

func (m featureSelectionModel) View() string {
	if m.done {
		return ""
	}
	s := fmt.Sprintf("\n%s\n\n", orangeStyle.Render("  Select Backend Features:"))

	for i, choice := range m.choices {
		cursor := "  "
		if m.cursor == i {
			cursor = orangeStyle.Render("❯ ")
		}

		checked := "[ ]"
		if choice.selected {
			checked = blueStyle.Render("[x]")
		}

		s += fmt.Sprintf("%s%s %s\n", cursor, checked, choice.title)
	}

	doneCursor := "  "
	doneStyle := lipgloss.NewStyle().Foreground(lipgloss.Color("240"))
	if m.cursor == len(m.choices) {
		doneCursor = orangeStyle.Render("❯ ")
		doneStyle = blueStyle.Bold(true)
	}
	s += fmt.Sprintf("\n%s%s\n", doneCursor, doneStyle.Render("[ Confirm & Create ]"))
	return s + "\n"
}

func selectFeatures() []string {
	p := tea.NewProgram(initialFeatureModel())
	m, err := p.Run()
	if err != nil {
		os.Exit(1)
	}

	var selected []string
	if finalModel, ok := m.(featureSelectionModel); ok {
		if !finalModel.done {
			os.Exit(0)
		}
		for _, item := range finalModel.choices {
			if item.selected {
				selected = append(selected, item.title)
			}
		}
	}
	return selected
}

func checkNpm() {
	if _, err := exec.LookPath("npm"); err != nil {
		fmt.Println(orangeStyle.Render("\n  [!] Error: Node.js/NPM is required for fullstack mode."))
		os.Exit(1)
	}
}

// --- Frontend Selection ---

type selectionModel struct {
	choices  []string
	cursor   int
	selected string
}

func initialSelectionModel() selectionModel {
	return selectionModel{
		choices: []string{"React", "Vue", "Svelte", "Solid", "Vanilla"},
	}
}

func (m selectionModel) Init() tea.Cmd { return nil }

func (m selectionModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "ctrl+c", "q":
			return m, tea.Quit
		case "up", "k":
			if m.cursor > 0 {
				m.cursor--
			}
		case "down", "j":
			if m.cursor < len(m.choices)-1 {
				m.cursor++
			}
		case "enter", " ":
			m.selected = m.choices[m.cursor]
			return m, tea.Quit
		}
	}
	return m, nil
}

func (m selectionModel) View() string {
	s := fmt.Sprintf("\n%s\n\n", orangeStyle.Render("  Select a Frontend Framework:"))
	for i, choice := range m.choices {
		if m.cursor == i {
			s += fmt.Sprintf("  %s %s\n", orangeStyle.Render("❯"), choice)
		} else {
			s += fmt.Sprintf("    %s\n", choice)
		}
	}
	return s + "\n"
}

func selectFrontend() string {
	p := tea.NewProgram(initialSelectionModel())
	m, err := p.Run()
	if err != nil {
		os.Exit(1)
	}
	if f, ok := m.(selectionModel); ok {
		return f.selected
	}
	return ""
}

func createProject(name string, frontend string, features []string) {
	dirs := []string{
		name,
		filepath.Join(name, "src"),
		filepath.Join(name, "src", "controllers"),
		filepath.Join(name, "include"),
		filepath.Join(name, "include", "controllers"),
		filepath.Join(name, "include", "models"),
	}
	for _, dir := range dirs {
		os.MkdirAll(dir, 0755)
	}

	files := map[string]string{
		"templates/main.cpp.tmpl":           filepath.Join(name, "src/main.cpp"),
		"templates/CMakeLists.txt.tmpl":     filepath.Join(name, "CMakeLists.txt"),
		"templates/gitignore.tmpl":          filepath.Join(name, ".gitignore"),
		"templates/dockerignore.tmpl":       filepath.Join(name, ".dockerignore"),
		"templates/Dockerfile.tmpl":         filepath.Join(name, "Dockerfile"),
		"templates/docker-compose.yml.tmpl": filepath.Join(name, "docker-compose.yml"),
		"templates/env.tmpl":                filepath.Join(name, ".env"),
	}

	data := ProjectData{ProjectName: name}
	for tmplPath, outPath := range files {
		tmplContent, _ := templatesFS.ReadFile(tmplPath)
		t, _ := template.New(tmplPath).Parse(string(tmplContent))
		f, _ := os.Create(outPath)
		t.Execute(f, data)
		f.Close()
	}

	origDir, _ := os.Getwd()
	os.Chdir(name)
	for _, f := range features {
		switch f {
		case "PostgreSQL":
			addDriver("postgres", "libpq-dev", "blaze::postgres", false)
		case "MySQL":
			addDriver("mysql", "libmariadb-dev", "blaze::mysql", false)
		case "Catch2 Testing":
			addTesting(false)
		}
	}
	os.Chdir(origDir)

	if frontend != "" {
		viteTemplate := "vanilla-ts"
		switch frontend {
		case "React":
			viteTemplate = "react-ts"
		case "Vue":
			viteTemplate = "vue-ts"
		case "Svelte":
			viteTemplate = "svelte-ts"
		case "Solid":
			viteTemplate = "solid-ts"
		}
		cmd := exec.Command("npm", "create", "vite@latest", "frontend", "--", "--template", viteTemplate)
		cmd.Dir = name
		cmd.Run()
	}

	fmt.Println(blueStyle.Render("\n  Project ready!"))
	fmt.Println(whiteStyle.Render(fmt.Sprintf("  cd %s", name)))
	fmt.Println(whiteStyle.Render("  blaze run\n"))
}
