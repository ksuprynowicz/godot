module Godot

interface [CNode]

state_type "MyState"

spec init(state, arguments :: [string]) :: {:ok :: label, state, code :: int}
                                  | {:fail :: label, state, result :: string}
spec call(state, method :: string, arg_0 :: payload, \
    arg_1 :: payload, arg_2 :: payload, \
    arg_3 :: payload, arg_4 :: payload, arg_5 :: payload)  :: {:ok :: label, state, :string :: label, result_string :: string}
                                  # | {:ok :: label, state, :float :: label, result_string :: float}
                                  | {:ok :: label, state, :int :: label, result_string :: int}
                                  | {:ok :: label, state, :bool ::label, result_string :: bool}