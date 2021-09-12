defmodule GodotState do
  defstruct pid: nil, last_tick: nil
end

defmodule Godot do
  use Unifex.Loader
  def init(args) do
      require Unifex.CNode
      {:ok, pid} = Unifex.CNode.start_link(:godot_elixir)
      state = %GodotState{pid: pid, last_tick: :os.system_time(:millisecond)}
      Unifex.CNode.call(state.pid, :init, [["godot"] ++ args])
      schedule_work() # Schedule work to be performed on start
      {:ok, state}
  end
end