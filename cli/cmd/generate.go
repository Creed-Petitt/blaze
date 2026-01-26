package cmd

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"text/template"

	"github.com/spf13/cobra"
)

type GenData struct {
	ClassName string
	FileName  string
	RoutePath string
}

var generateCmd = &cobra.Command{
	Use:     "generate [type] [name]",
	Aliases: []string{"g"},
	Short:   "Generate boilerplate code for controllers or models",
}

var controllerGenCmd = &cobra.Command{
	Use:   "controller [name]",
	Short: "Generate a new controller",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		name := args[0]
		generateFile(name, "controller")
	},
}

var modelGenCmd = &cobra.Command{
	Use:   "model [name]",
	Short: "Generate a new model",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		name := args[0]
		generateFile(name, "model")
	},
}

func init() {
	rootCmd.AddCommand(generateCmd)
	generateCmd.AddCommand(controllerGenCmd)
	generateCmd.AddCommand(modelGenCmd)
}

func generateFile(name string, genType string) {
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println(orangeStyle.Render("Error: No Blaze project found. Run this in the project root."))
		return
	}

	className := toCamelCase(name)
	fileName := strings.ToLower(name)

	data := GenData{
		ClassName: className,
		FileName:  fileName,
		RoutePath: fileName,
	}

	if genType == "controller" {
		data.ClassName += "Controller"
		writeGenTemplate("templates/controller.h.tmpl", filepath.Join("include/controllers", fileName+"_controller.h"), data)
		writeGenTemplate("templates/controller.cpp.tmpl", filepath.Join("src/controllers", fileName+"_controller.cpp"), data)
		fmt.Printf(blueStyle.Render("  ✓ Created %sController in controllers/%s_controller.h/cpp\n"), className, fileName)
		fmt.Printf(whiteStyle.Render("  ! Don't forget to register it in main.cpp:\n"))
		fmt.Printf(dimStyle.Render("    controllers::%sController::register_routes(app);\n\n"), className)
	} else if genType == "model" {
		writeGenTemplate("templates/model.h.tmpl", filepath.Join("include/models", fileName+".h"), data)
		fmt.Printf(blueStyle.Render("  ✓ Created model %s in models/%s.h\n\n"), className, fileName)
	}
}

func writeGenTemplate(tmplPath string, outPath string, data GenData) {
	tmplContent, err := templatesFS.ReadFile(tmplPath)
	if err != nil {
		fmt.Printf("Error reading template: %v\n", err)
		return
	}
	t, _ := template.New(tmplPath).Parse(string(tmplContent))
	f, _ := os.Create(outPath)
	t.Execute(f, data)
	f.Close()
}

func toCamelCase(s string) string {
	parts := strings.Split(s, "_")
	for i, part := range parts {
		if len(part) > 0 {
			parts[i] = strings.ToUpper(part[:1]) + strings.ToLower(part[1:])
		}
	}
	return strings.Join(parts, "")
}
