package cmd

import (
	"fmt"

	"github.com/spf13/cobra"
)

// initCmd represents the init command
var initCmd = &cobra.Command{
	Use:   "init [name]",
	Short: "Initialize a new Blaze project",
	Long: `Scaffolds a new C++ project with CMake, Docker, and source files.
	Example: blaze init my-api`,
	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		projectName := args[0]
		createProject(projectName)
	},
}

func init() {
	rootCmd.AddCommand(initCmd)
}

func createProject(name string) {
	fmt.Printf("TODO: Scaffold project '%s'\n", name)
}
