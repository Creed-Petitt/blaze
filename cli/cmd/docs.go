package cmd

import (
	"fmt"
	"github.com/charmbracelet/lipgloss"
	"github.com/spf13/cobra"
)

var docsCmd = &cobra.Command{
	Use:   "docs",
	Short: "Show information about API documentation and Swagger UI",
	Run: func(cmd *cobra.Command, args []string) {
		var (
			titleStyle = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#FF4C4C"))
			linkStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#04B575")).Underline(true)
			textStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF"))
		)

		fmt.Println(titleStyle.Render("\n  Blaze Interactive Documentation"))
		fmt.Println(textStyle.Render("  ------------------------------"))
		fmt.Println(textStyle.Render("\n  Blaze automatically generates OpenAPI 3.0 specs for your API."))
		fmt.Println(textStyle.Render("  No configuration or annotations are required.\n"))

		fmt.Println(textStyle.Render("  [+] Swagger UI:   ") + linkStyle.Render("http://localhost:8080/docs"))
		fmt.Println(textStyle.Render("  [+] OpenAPI Spec: ") + linkStyle.Render("http://localhost:8080/openapi.json"))

		fmt.Println(textStyle.Render("\n  To include models in your docs, use the BLAZE_MODEL macro."))
		fmt.Println(textStyle.Render("  To document path/query params, use the Path<T> and Query<T> wrappers.\n"))
	},
}

func init() {
	rootCmd.AddCommand(docsCmd)
}
