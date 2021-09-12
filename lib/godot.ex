defmodule GodotState do
  defstruct pid: nil, last_tick: :os.system_time(:millisecond), result: []
end

defmodule Godot do
  use Unifex.Loader
  @godot_timeout 5_000
  @godot_frame 1.0 / 20.0 * 1000.0
  use GenServer
  require Unifex.CNode

  def start_link do
      GenServer.start_link(__MODULE__, %{})
  end

  def iteration(state) do  
    Unifex.CNode.call(state.pid, :iteration, [], @godot_timeout)
    state = %GodotState{pid: state.pid, last_tick: :os.system_time(:millisecond), result: []}
    {:ok, state}
  end

  def stop(state) do  
    Unifex.CNode.stop(state.pid)
    state = %GodotState{pid: nil, last_tick: :os.system_time(:millisecond), result: []}
    {:ok, state}
  end

  def call(state, args) do  
    res = Unifex.CNode.call(state.pid, :call, args, @godot_timeout)
    state = %GodotState{pid: state.pid, last_tick: :os.system_time(:millisecond), result: [res]}
    {:ok, state}
  end

  def init(args) do
      {:ok, pid} = Unifex.CNode.start_link(:godot_elixir)
      Unifex.CNode.call(pid, :init, [["godot"]] ++ args, @godot_timeout)
      state = %GodotState{pid: pid, last_tick: :os.system_time(:millisecond), result: []}
      {:ok, state}
  end

  def handle_info(:work, state) do   
      state = iteration(state)
      schedule_work()
      {:noreply, state}
  end

  defp schedule_work() do
      Process.send_after(self(), :work, @godot_frame)
  end
end