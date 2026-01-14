package cmd

import (
	"os"

	"github.com/spf13/cobra"
)

// rootCmd represents the base command when called without any subcommands
var rootCmd = &cobra.Command{
	Use:   "blaze",
	Short: "A blazing fast C++ web framework CLI",
	Long: `Blaze is a CLI tool for scaffolding, building, and deploying
modern C++ web applications using the Blaze framework.`,
}

// Execute adds all child commands to the root command and sets flags appropriately
func Execute() {
	err := rootCmd.Execute()
	if err != nil {
		os.Exit(1)
	}
}

func init() {
	// TODO: Define flags and configuration settings.

	// rootCmd.PersistentFlags().StringVar(&cfgFile, "config", "", "config file (default: $HOME/.blaze.yaml)")
	// rootCmd.Flags().BoolP("toggle", "t", false, "Help message for toggle")
}
