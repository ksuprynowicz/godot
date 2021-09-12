defmodule GodotState do
  defstruct pid: nil, last_tick: nil
end

defmodule Godot do
  use Unifex.Loader
  use GenServer

  def start_link do
      GenServer.start_link(__MODULE__, %{})
  end

  def init(_state) do
      require Unifex.CNode
      {:ok, pid} = Unifex.CNode.start_link(:godot_elixir)
      state = %GodotState{pid: pid, last_tick: :os.system_time(:millisecond)}
      Unifex.CNode.call(state.pid, :init, [["godot", "--verbose"]])
      schedule_work() # Schedule work to be performed on start
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
      Process.send_after(self(), :work, 33) # In 2 hours
  end
end