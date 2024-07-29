PROGRAM My_Program
  VAR
    temp AT %IW0 : UINT;
    temp_filtered AT %QW6 : UINT;
    Alarm AT %QX0.5 : BOOL;
    min_sp AT %QW1 : UINT := 18333;
    max_sp AT %QW2 : UINT := 23889;
    mode_register AT %QW3 : INT := 2;
    heat_switch AT %QX1.4 : BOOL;
    cool_switch AT %QX1.5 : BOOL;
    heat AT %QX0.0 : BOOL;
    cool AT %QX0.1 : BOOL;
    weather AT %QX0.2 : BOOL;
    heat_man AT %QX1.0 : BOOL;
    heat_auto AT %QX1.1 : BOOL;
    cool_man AT %QX1.2 : BOOL;
    cool_auto AT %QX1.3 : BOOL;
  END_VAR
  VAR
    TP0 : TP;
    TP1 : TP;
    _TMP_LE98_OUT : BOOL;
    _TMP_GE103_OUT : BOOL;
    _TMP_EQ49_OUT : BOOL;
    _TMP_EQ38_OUT : BOOL;
    _TMP_EQ1_OUT : BOOL;
    _TMP_GE16_OUT : BOOL;
    _TMP_LE11_OUT : BOOL;
    _TMP_EQ20_OUT : BOOL;
    _TMP_EQ18_OUT : BOOL;
  END_VAR

  _TMP_LE98_OUT := LE(temp_filtered, min_sp);
  IF _TMP_LE98_OUT THEN
    heat_auto := TRUE; (*set*)
  END_IF;
  _TMP_GE103_OUT := GE(temp_filtered, max_sp);
  IF _TMP_GE103_OUT THEN
    heat_auto := FALSE; (*reset*)
  END_IF;
  heat_man := heat_switch;
  cool_man := cool_switch;
  _TMP_EQ49_OUT := EQ(mode_register, 1);
  _TMP_EQ38_OUT := EQ(mode_register, 2);
  heat := heat_auto AND _TMP_EQ38_OUT OR heat_man AND _TMP_EQ49_OUT;
  temp_filtered := temp;
  _TMP_EQ1_OUT := EQ(mode_register, 0);
  IF _TMP_EQ1_OUT THEN
    heat := FALSE; (*reset*)
  END_IF;
  IF _TMP_EQ1_OUT THEN
    cool := FALSE; (*reset*)
  END_IF;
  _TMP_GE16_OUT := GE(temp_filtered, max_sp);
  IF _TMP_GE16_OUT THEN
    cool_auto := TRUE; (*set*)
  END_IF;
  _TMP_LE11_OUT := LE(temp_filtered, min_sp);
  IF _TMP_LE11_OUT THEN
    cool_auto := FALSE; (*reset*)
  END_IF;
  _TMP_EQ20_OUT := EQ(mode_register, 1);
  _TMP_EQ18_OUT := EQ(mode_register, 3);
  cool := cool_auto AND _TMP_EQ18_OUT OR cool_man AND _TMP_EQ20_OUT;
  TP0(IN := heat_switch, PT := T#500ms);
  IF TP0.Q THEN
    cool_switch := FALSE; (*reset*)
  END_IF;
  TP1(IN := cool_switch, PT := T#500ms);
  IF TP1.Q THEN
    heat_switch := FALSE; (*reset*)
  END_IF;
END_PROGRAM

FUNCTION_BLOCK Level_Filtering
  VAR_INPUT
    raw_level : INT;
  END_VAR
  VAR
    level_real : REAL;
    avg : LREAL;
    level_array : ARRAY [1..20] OF REAL;
    i : INT;
    current_array_pos : INT := 1;
  END_VAR
  VAR_OUTPUT
    filtered_level : INT;
  END_VAR

  (* Level Conversion *)
  level_real := INT_TO_REAL(raw_level);
  level_real := level_real - 107.0;
  level_real := level_real * 0.483091;
  level_real := level_real * 100.0;

  (* Store value in average array *)
  level_array[current_array_pos] := level_real;
  current_array_pos := current_array_pos + 1;

  (* Prevent array_pos overflow *)
  if current_array_pos > 20 then
    current_array_pos := 1;
  end_if;

  (* Calculate average *)
  avg := 0.0;
  i := 1;
  while (i < 21) do
    avg := avg + REAL_TO_LREAL(level_array[i]);
    i := i + 1;
  end_while;
  avg := avg / 20.0;

  (* Set output value *)
  filtered_level := LREAL_TO_INT(avg);
END_FUNCTION_BLOCK


CONFIGURATION Config0

  RESOURCE Res0 ON PLC
    TASK TaskMain(INTERVAL := T#50ms,PRIORITY := 0);
    PROGRAM Inst0 WITH TaskMain : My_Program;
  END_RESOURCE
END_CONFIGURATION
