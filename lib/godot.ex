defmodule GodotState do
  defstruct pid: nil, last_tick: nil
end


defmodule Godot do
  use Unifex.Loader
  @godot_timeout 60_000
  def init(args) do
      require Unifex.CNode
      {:ok, pid} = Unifex.CNode.start_link(:godot_elixir)
      Unifex.CNode.call(pid, :init, [["godot"] ++ args], @godot_timeout)
      state = %GodotState{pid: pid, last_tick: :os.system_time(:millisecond)}
      {:ok, state}
  end
  def call(state, method) do
      Unifex.CNode.call(state.pid, :call, method, @godot_timeout)
      state = %GodotState{pid: state.pid, last_tick: :os.system_time(:millisecond)}
      {:ok, state}
  end
  def stop(state) do
      Unifex.CNode.stop(state.pid)
      state = %GodotState{pid: state.pid, last_tick: :os.system_time(:millisecond)}
      {:ok, state}
  end
end