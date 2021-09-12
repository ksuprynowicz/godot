defmodule GodotState do
  defstruct pid: nil, last_tick: nil
end

defmodule Godot do
  use Unifex.Loader
  @godot_timeout 5000
  @godot_frame 1.0 / 20.0 * 1000.0
  use GenServer

  def start_link do
      GenServer.start_link(__MODULE__, %{})
  end

  def init(args) do
      require Unifex.CNode
      {:ok, pid} = Unifex.CNode.start_link(:godot_elixir)
      Unifex.CNode.call(pid, :init, [["godot"]] ++ args, @godot_timeout)
      state = %GodotState{pid: pid, last_tick: :os.system_time(:millisecond)}
      {:ok, state}
  end

  def handle_info(:work, state) do
      current_tick = :os.system_time(:millisecond)
      tick = (current_tick - state.last_tick) / 1000
      Unifex.CNode.call(state.pid, :iteration, [tick])
      schedule_work()
      state = %GodotState{pid: state.pid, last_tick: :os.system_time(:millisecond)}
      {:noreply, state}
  end

  defp schedule_work() do
      Process.send_after(self(), :work, @godot_frame)
  end
end