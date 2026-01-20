package cmd

import (
	"embed"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"text/template"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

//go:embed templates/*
var templatesFS embed.FS

var fullstackMode bool

// initCmd represents the init command
var initCmd = &cobra.Command{
	Use:   "init [name]",
	Short: "Initialize a new Blaze project",
	Long: `Scaffolds a new C++ project with CMake, Docker, and source files.
Use --fullstack to include a React/Vue/Svelte frontend.`, 
	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		projectName := args[0]
		
		if fullstackMode {
			checkNpm()
			framework := selectFrontend()
			if framework == "" {
				fmt.Println("Selection cancelled.")
				os.Exit(0)
			}
			createProject(projectName, framework)
		} else {
			createProject(projectName, "")
		}
	},
}

func init() {
	rootCmd.AddCommand(initCmd)
	initCmd.Flags().BoolVarP(&fullstackMode, "fullstack", "f", false, "Initialize with a Frontend (React/Vue/Svelte)")
}

type ProjectData struct {
	ProjectName string
}

func checkNpm() {
	_, err := exec.LookPath("npm")
	if err != nil {
		fmt.Println(lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true).Render("\n  [!] Error: Node.js/NPM is required for fullstack mode."))
		fmt.Println("  Please install Node.js from https://nodejs.org\n")
		os.Exit(1)
	}
}

// BubbleTea Selection Logic ---

type item struct {
	title, desc string
}

type selectionModel struct {
	choices  []item
	cursor   int
	selected string
}

func initialSelectionModel() selectionModel {
	return selectionModel{
		choices: []item{
			{title: "React", desc: "The most popular library (TypeScript)"},
			{title: "Vue", desc: "The progressive framework (TypeScript)"},
			{title: "Svelte", desc: "Cybernetically enhanced web apps (TypeScript)"},
			{title: "Solid", desc: "Simple and performant reactivity (TypeScript)"},
			{title: "Vanilla", desc: "Plain TypeScript (No Framework)"},
		},
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
			m.selected = m.choices[m.cursor].title
			return m, tea.Quit
		}
	}
	return m, nil
}

func (m selectionModel) View() string {
	// Header: Style text only, handle newlines manually to avoid alignment bugs
	headerText := lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C")).Render("  Select a Frontend Framework:")
	s := fmt.Sprintf("\n%s\n\n", headerText)

	for i, choice := range m.choices {
		if m.cursor == i {
			// Selected: "  > Title" (Red)
			cursor := lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true).Render("❯")
			title := lipgloss.NewStyle().Foreground(lipgloss.Color("#FF4C4C")).Bold(true).Render(choice.title)
			s += fmt.Sprintf("  %s %s\n", cursor, title)
		} else {
			// Unselected: "    Title" (White)
			title := lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF")).Render(choice.title)
			s += fmt.Sprintf("    %s\n", title)
		}
	}
	s += "\n  (Press q to quit)\n"
	return s
}

func selectFrontend() string {
	p := tea.NewProgram(initialSelectionModel())
	m, err := p.Run()
	if err != nil {
		fmt.Printf("Alas, there's been an error: %v", err)
		os.Exit(1)
	}

	if finalModel, ok := m.(selectionModel); ok {
		return finalModel.selected
	}
	return ""
}

// Scaffolding Logic

func createProject(name string, frontend string) {
	var (
		titleStyle = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C")) // Blaze Red
		checkStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575"))            // Green
		textStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF"))            // White
	)

	fmt.Println(titleStyle.Render(fmt.Sprintf("\n  Scaffolding project '%s'...", name)))

	dirs := []string{
		name,
		filepath.Join(name, "src"),
		filepath.Join(name, "include"),
	}

	for _, dir := range dirs {
		if err := os.MkdirAll(dir, 0755); err != nil {
			fmt.Printf("Error creating directory %s: %v\n", dir, err)
			os.Exit(1)
		}
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
		tmplContent, err := templatesFS.ReadFile(tmplPath)
		if err == nil {
			t, _ := template.New(tmplPath).Parse(string(tmplContent))
			f, _ := os.Create(outPath)
			t.Execute(f, data)
			f.Close()
		}
	}

	fmt.Println(checkStyle.Render("  [+] Backend structure created"))

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

		// Run npm create vite (Silent)
		cmd := exec.Command("npm", "create", "vite@latest", "frontend", "--", "--template", viteTemplate)
		cmd.Dir = name
		// Suppress stdout, but keep stderr for errors
		cmd.Stdout = nil
		cmd.Stderr = os.Stderr

		if err := cmd.Run(); err != nil {
			fmt.Printf("  Error creating frontend: %v\n", err)
		} else {
			fmt.Println(checkStyle.Render(fmt.Sprintf("  [+] Frontend (%s) initialized in /frontend", frontend)))
		}
	}

	fmt.Println("")
	fmt.Println(textStyle.Render("  Project ready! To get started:"))
	fmt.Println("")
	fmt.Println(textStyle.Render(fmt.Sprintf("  cd %s", name)))
	if frontend != "" {
		fmt.Println(textStyle.Render("  cd frontend && npm install && cd .."))
		fmt.Println(textStyle.Render("  blaze dev  (Runs Backend + Frontend)"))
	} else {
		fmt.Println(textStyle.Render("  blaze run"))
	}
	fmt.Println("")
}