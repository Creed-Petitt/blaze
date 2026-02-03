package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"strings"

	"github.com/spf13/cobra"
)

var addCmd = &cobra.Command{
	Use:   "add [feature]",
	Short: "Add features or drivers to an existing Blaze project",
}

var addFrontendCmd = &cobra.Command{
	Use:   "frontend",
	Short: "Add a Vite-based frontend to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addFrontend()
	},
}

var addMysqlCmd = &cobra.Command{
	Use:   "mysql",
	Short: "Add MySQL driver support to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addDriver("mysql", "libmariadb-dev", "blaze::mysql", false)
	},
}

var addPostgresCmd = &cobra.Command{
	Use:   "postgres",
	Short: "Add PostgreSQL driver support to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addDriver("postgres", "libpq-dev", "blaze::postgres", false)
	},
}

var addTestCmd = &cobra.Command{
	Use:   "test",
	Short: "Add a Catch2 testing suite to this project",
	Run: func(cmd *cobra.Command, args []string) {
		addTesting(false)
	},
}

func init() {
	rootCmd.AddCommand(addCmd)
	addCmd.AddCommand(addFrontendCmd)
	addCmd.AddCommand(addMysqlCmd)
	addCmd.AddCommand(addPostgresCmd)
	addCmd.AddCommand(addTestCmd)
}

func addDriver(name, pkgName, libName string, silent bool) {
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		if !silent {
			fmt.Println(blueStyle.Render("Error: No Blaze project found."))
		}
		return
	}

	content, _ := os.ReadFile("CMakeLists.txt")
	cmakeStr := string(content)

	if strings.Contains(cmakeStr, libName) {
		if !silent {
			fmt.Println(blueStyle.Render(fmt.Sprintf("  ✓ %s is already configured", name)))
		}
		return
	}

	marker := "# [[BLAZE_DRIVERS_START]]"
	if strings.Contains(cmakeStr, marker) {
		newCmake := strings.ReplaceAll(cmakeStr, marker, marker+"\n    "+libName)
		os.WriteFile("CMakeLists.txt", []byte(newCmake), 0644)
	} else {
		newCmake := strings.ReplaceAll(cmakeStr, "blaze::core", "blaze::core\n    "+libName)
		os.WriteFile("CMakeLists.txt", []byte(newCmake), 0644)
	}

	if !silent {
		// Quick system check for the driver library
		pkgConfigName := strings.TrimSuffix(pkgName, "-dev")
		if pkgConfigName == "libmariadb" {
			pkgConfigName = "libmariadb" // Match doctor.go
		}
		if pkgConfigName == "libpq" {
			pkgConfigName = "libpq"
		}

		check := exec.Command("pkg-config", "--exists", pkgConfigName)
		if err := check.Run(); err != nil {
			fmt.Printf(blueStyle.Render("  [!] Warning: %s dev libs not found on system.\n"), name)
			fmt.Printf(dimStyle.Render("      You may need to run 'blaze doctor --fix' or install %s manually.\n"), pkgName)
		}

		fmt.Println(blueStyle.Render(fmt.Sprintf("  ✓ Added %s support", name)))
	}
}

func addTesting(silent bool) {
	os.MkdirAll("tests", 0755)

	testFile := "#include <blaze/app.h>\n#include <catch2/catch_test_macros.hpp>\n\nusing namespace blaze;\n\nTEST_CASE(\"Basic Math\", \"[math]\") {\n    REQUIRE(1 + 1 == 2);\n}\n"
	os.WriteFile("tests/test_main.cpp", []byte(testFile), 0644)

	content, _ := os.ReadFile("CMakeLists.txt")
	cmakeStr := string(content)

	if !strings.Contains(cmakeStr, "enable_testing()") {
		// Detect existing drivers to inject into the test suite
		existingDrivers := ""
		if strings.Contains(cmakeStr, "blaze::postgres") {
			existingDrivers += "\n    blaze::postgres"
		}
		if strings.Contains(cmakeStr, "blaze::mysql") {
			existingDrivers += "\n    blaze::mysql"
		}

		catch2Block := fmt.Sprintf(`
# Download Catch2 for Testing
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# Testing Suite
enable_testing()
add_executable(blaze_tests tests/test_main.cpp)
target_link_libraries(blaze_tests PRIVATE 
    blaze::core
    # [[BLAZE_DRIVERS_START]]%s
    # [[BLAZE_DRIVERS_END]]
    Catch2::Catch2WithMain
)`, existingDrivers)
		os.WriteFile("CMakeLists.txt", []byte(cmakeStr+catch2Block), 0644)
	}

	if !silent {
		fmt.Println(blueStyle.Render("  ✓ Setup testing suite"))
	}
}

func addFrontend() {
	if _, err := os.Stat("CMakeLists.txt"); os.IsNotExist(err) {
		fmt.Println(blueStyle.Render("Error: No Blaze project found."))
		return
	}
	if _, err := os.Stat("frontend"); err == nil {
		fmt.Println(blueStyle.Render("Error: A 'frontend' directory already exists."))
		return
	}
	checkNpm()
	framework := selectFrontend()
	if framework == "" {
		return
	}

	fmt.Println(blueStyle.Render(fmt.Sprintf("\n  ✓ Adding %s frontend...", framework)))

	viteTemplate := "vanilla-ts"
	switch framework {
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
	cmd.Dir = "."
	cmd.Stdout = nil
	cmd.Stderr = os.Stderr
	cmd.Run()

	fmt.Println(blueStyle.Render(fmt.Sprintf("  ✓ Frontend (%s) initialized", framework)))
}
