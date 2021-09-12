defmodule Godot.Mixfile do
  use Mix.Project

  def project do
    [
      app: :godot,
      compilers: [:elixir_make, :unifex, :bundlex] ++ Mix.compilers,
      version: "0.1.0",
      deps: deps(),
      extra_applications: [:logger],
      package: package(),
   ]
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp deps() do
    [
      {:unifex, "~> 0.7.0"},
      {:elixir_make, "~> 0.4", runtime: false},
    ]
  end

  defp package do
    [
      licenses: ["MIT"],
      files: [
        "lib", "LICENSE.txt", "mix.exs", "README.md",
        "bin", "c_src", "c_src/*.md", "Makefile"],
      links: %{"GitHub" => "https://github.com/V-Sekai/godot/tree/elixir"}
    ]
  end
end
