PROGRAM My_Program
  VAR
    temp AT %IW0 : INT := 7000;
    temp_outside_read AT %IW1 : INT := 3000;
    heat_on AT %QX0.0 : BOOL := False;
    cool_on AT %QX0.1 : BOOL := False;
    Alarm_LOW_LOW AT %QX0.3 : BOOL;
    Alarm_LOW AT %QX0.4 : BOOL;
    Alarm_High AT %QX0.5 : BOOL;
    Alarm_High_High AT %QX0.6 : BOOL;
    heat_manual AT %QX1.0 : BOOL;
    cool_manual AT %QX1.1 : BOOL;
    HVAC_mode AT %QW0 : UINT := 1;
    min_sp AT %QW2 : INT := 6500;
    max_sp AT %QW3 : INT := 7500;
    temp_outside AT %QW5 : INT := 3000;
    temp_inside AT %QW6 : INT := 7000;
    abs_min_sp AT %QW8 : INT := 5500;
    abs_max_sp AT %QW9 : INT := 8500;
  END_VAR
  VAR
    _TMP_EQ1_OUT : BOOL;
    _TMP_EQ5_OUT : BOOL;
    _TMP_EQ137_OUT : BOOL;
    _TMP_SUB191_OUT : INT;
    _TMP_LT8_ENO : BOOL;
    _TMP_LT8_OUT : BOOL;
    _TMP_EQ34_OUT : BOOL;
    _TMP_ADD193_OUT : INT;
    _TMP_GE44_ENO : BOOL;
    _TMP_GE44_OUT : BOOL;
    _TMP_EQ21_OUT : BOOL;
    _TMP_SUB102_OUT : INT;
    _TMP_LT40_ENO : BOOL;
    _TMP_LT40_OUT : BOOL;
    _TMP_EQ38_OUT : BOOL;
    _TMP_GE46_ENO : BOOL;
    _TMP_GE46_OUT : BOOL;
    _TMP_EQ48_OUT : BOOL;
    _TMP_LT55_ENO : BOOL;
    _TMP_LT55_OUT : BOOL;
    _TMP_EQ45_OUT : BOOL;
    _TMP_ADD104_OUT : INT;
    _TMP_GE58_ENO : BOOL;
    _TMP_GE58_OUT : BOOL;
    _TMP_EQ66_OUT : BOOL;
    _TMP_EQ75_OUT : BOOL;
    _TMP_LT92_OUT : BOOL;
    _TMP_GT62_OUT : BOOL;
    _TMP_AND93_OUT : BOOL;
    _TMP_LE150_OUT : BOOL;
    _TMP_GT174_OUT : BOOL;
    _TMP_AND175_OUT : BOOL;
    _TMP_LE81_OUT : BOOL;
    _TMP_GE84_OUT : BOOL;
    _TMP_LT176_OUT : BOOL;
    _TMP_AND29_OUT : BOOL;
    _TMP_GE87_OUT : BOOL;
  END_VAR

  temp_inside := temp;
  temp_outside := temp_outside_read;
  _TMP_EQ1_OUT := EQ(HVAC_mode, 0);
  IF _TMP_EQ1_OUT THEN
    heat_on := FALSE; (*reset*)
  END_IF;
  _TMP_EQ5_OUT := EQ(HVAC_mode, 0);
  IF _TMP_EQ5_OUT THEN
    cool_on := FALSE; (*reset*)
  END_IF;
  _TMP_EQ137_OUT := EQ(HVAC_mode, 1);
  _TMP_SUB191_OUT := SUB(min_sp, 100);
  _TMP_LT8_OUT := LT(EN := _TMP_EQ137_OUT, IN1 := temp_inside, IN2 := _TMP_SUB191_OUT, ENO => _TMP_LT8_ENO);
  heat_on := _TMP_LT8_OUT;
  IF _TMP_LT8_OUT THEN
    cool_on := FALSE; (*reset*)
  END_IF;
  _TMP_EQ34_OUT := EQ(HVAC_mode, 1);
  _TMP_ADD193_OUT := ADD(max_sp, 100);
  _TMP_GE44_OUT := GE(EN := _TMP_EQ34_OUT, IN1 := temp_inside, IN2 := _TMP_ADD193_OUT, ENO => _TMP_GE44_ENO);
  cool_on := _TMP_GE44_OUT;
  IF _TMP_GE44_OUT THEN
    heat_on := FALSE; (*reset*)
  END_IF;
  _TMP_EQ21_OUT := EQ(HVAC_mode, 2);
  _TMP_SUB102_OUT := SUB(min_sp, 100);
  _TMP_LT40_OUT := LT(EN := _TMP_EQ21_OUT, IN1 := temp_inside, IN2 := _TMP_SUB102_OUT, ENO => _TMP_LT40_ENO);
  IF _TMP_LT40_OUT THEN
    heat_on := TRUE; (*set*)
  END_IF;
  _TMP_EQ38_OUT := EQ(HVAC_mode, 2);
  _TMP_GE46_OUT := GE(EN := _TMP_EQ38_OUT, IN1 := temp_inside, IN2 := max_sp, ENO => _TMP_GE46_ENO);
  IF _TMP_GE46_OUT THEN
    heat_on := FALSE; (*reset*)
  END_IF;
  _TMP_EQ48_OUT := EQ(HVAC_mode, 3);
  _TMP_LT55_OUT := LT(EN := _TMP_EQ48_OUT, IN1 := temp_inside, IN2 := min_sp, ENO => _TMP_LT55_ENO);
  IF _TMP_LT55_OUT THEN
    cool_on := FALSE; (*reset*)
  END_IF;
  _TMP_EQ45_OUT := EQ(HVAC_mode, 3);
  _TMP_ADD104_OUT := ADD(max_sp, 100);
  _TMP_GE58_OUT := GE(EN := _TMP_EQ45_OUT, IN1 := temp_inside, IN2 := _TMP_ADD104_OUT, ENO => _TMP_GE58_ENO);
  IF _TMP_GE58_OUT THEN
    cool_on := TRUE; (*set*)
  END_IF;
  _TMP_EQ66_OUT := EQ(HVAC_mode, 4);
  IF NOT(heat_manual) AND cool_manual AND _TMP_EQ66_OUT THEN
    cool_on := TRUE; (*set*)
  END_IF;
  IF NOT(heat_manual) AND cool_manual AND _TMP_EQ66_OUT THEN
    heat_on := FALSE; (*reset*)
  END_IF;
  _TMP_EQ75_OUT := EQ(HVAC_mode, 4);
  IF NOT(cool_manual) AND heat_manual AND _TMP_EQ75_OUT THEN
    heat_on := TRUE; (*set*)
  END_IF;
  IF NOT(cool_manual) AND heat_manual AND _TMP_EQ75_OUT THEN
    cool_on := FALSE; (*reset*)
  END_IF;
  _TMP_LT92_OUT := LT(temp_inside, max_sp);
  _TMP_GT62_OUT := GT(temp_inside, min_sp);
  _TMP_AND93_OUT := AND(_TMP_LT92_OUT, _TMP_GT62_OUT);
  IF _TMP_AND93_OUT THEN
    Alarm_High_High := FALSE; (*reset*)
  END_IF;
  IF _TMP_AND93_OUT THEN
    Alarm_High := FALSE; (*reset*)
  END_IF;
  IF _TMP_AND93_OUT THEN
    Alarm_LOW := FALSE; (*reset*)
  END_IF;
  IF _TMP_AND93_OUT THEN
    Alarm_LOW_LOW := FALSE; (*reset*)
  END_IF;
  _TMP_LE150_OUT := LE(temp_inside, min_sp);
  _TMP_GT174_OUT := GT(temp_inside, abs_min_sp);
  _TMP_AND175_OUT := AND(_TMP_LE150_OUT, _TMP_GT174_OUT);
  IF _TMP_AND175_OUT THEN
    Alarm_LOW_LOW := FALSE; (*reset*)
  END_IF;
  IF _TMP_AND175_OUT THEN
    Alarm_LOW := TRUE; (*set*)
  END_IF;
  _TMP_LE81_OUT := LE(temp_inside, abs_min_sp);
  IF _TMP_LE81_OUT THEN
    Alarm_LOW_LOW := TRUE; (*set*)
  END_IF;
  IF _TMP_LE81_OUT THEN
    Alarm_LOW := FALSE; (*reset*)
  END_IF;
  _TMP_GE84_OUT := GE(temp_inside, max_sp);
  _TMP_LT176_OUT := LT(temp_inside, abs_max_sp);
  _TMP_AND29_OUT := AND(_TMP_GE84_OUT, _TMP_LT176_OUT);
  IF _TMP_AND29_OUT THEN
    Alarm_High_High := FALSE; (*reset*)
  END_IF;
  IF _TMP_AND29_OUT THEN
    Alarm_High := TRUE; (*set*)
  END_IF;
  _TMP_GE87_OUT := GE(temp_inside, abs_max_sp);
  IF _TMP_GE87_OUT THEN
    Alarm_High_High := TRUE; (*set*)
  END_IF;
  IF _TMP_GE87_OUT THEN
    Alarm_High := FALSE; (*reset*)
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
