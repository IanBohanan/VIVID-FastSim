PROGRAM My_Program
  VAR
    Level AT %IW0 : INT;
    min_sp AT %QW1 : INT;
    max_sp AT %QW2 : INT;
    mode_register AT %QW3 : INT;
    pump_register AT %QW4 : INT;
    valve_register AT %QW5 : INT;
    pump AT %QX0.0 : BOOL;
    valve AT %QX0.1 : BOOL;
  END_VAR
  VAR
    EQ83_ENO : BOOL;
    EQ83_OUT : BOOL;
    EQ44_ENO : BOOL;
    EQ44_OUT : BOOL;
    EQ7_ENO : BOOL;
    EQ7_OUT : BOOL;
    LE87_ENO : BOOL;
    LE87_OUT : BOOL;
    EQ14_ENO : BOOL;
    EQ14_OUT : BOOL;
    EQ11_ENO : BOOL;
    EQ11_OUT : BOOL;
    EQ2_ENO : BOOL;
    EQ2_OUT : BOOL;
    GE93_ENO : BOOL;
    GE93_OUT : BOOL;
    EQ33_ENO : BOOL;
    EQ33_OUT : BOOL;
    EQ17_ENO : BOOL;
    EQ17_OUT : BOOL;
    EQ27_ENO : BOOL;
    EQ27_OUT : BOOL;
    GE31_ENO : BOOL;
    GE31_OUT : BOOL;
    EQ41_ENO : BOOL;
    EQ41_OUT : BOOL;
    EQ21_ENO : BOOL;
    EQ21_OUT : BOOL;
    EQ36_ENO : BOOL;
    EQ36_OUT : BOOL;
    LE48_ENO : BOOL;
    LE48_OUT : BOOL;
  END_VAR

  EQ83_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 1, ENO => EQ83_ENO);
  EQ44_OUT := EQ(EN := EQ83_OUT, IN1 := pump_register, IN2 := 1, ENO => EQ44_ENO);
  EQ7_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 2, ENO => EQ7_ENO);
  LE87_OUT := LE(EN := EQ7_OUT, IN1 := Level, IN2 := min_sp, ENO => LE87_ENO);
  IF EQ44_OUT OR LE87_OUT THEN
    pump := TRUE; (*set*)
  END_IF;
  EQ14_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 1, ENO => EQ14_ENO);
  EQ11_OUT := EQ(EN := EQ14_OUT, IN1 := pump_register, IN2 := 0, ENO => EQ11_ENO);
  EQ2_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 2, ENO => EQ2_ENO);
  GE93_OUT := GE(EN := EQ2_OUT, IN1 := Level, IN2 := max_sp, ENO => GE93_ENO);
  IF EQ11_OUT OR GE93_OUT THEN
    pump := FALSE; (*reset*)
  END_IF;
  EQ33_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 1, ENO => EQ33_ENO);
  EQ17_OUT := EQ(EN := EQ33_OUT, IN1 := valve_register, IN2 := 1, ENO => EQ17_ENO);
  EQ27_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 2, ENO => EQ27_ENO);
  GE31_OUT := GE(EN := EQ27_OUT, IN1 := Level, IN2 := max_sp, ENO => GE31_ENO);
  IF EQ17_OUT OR GE31_OUT THEN
    valve := TRUE; (*set*)
  END_IF;
  EQ41_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 1, ENO => EQ41_ENO);
  EQ21_OUT := EQ(EN := EQ41_OUT, IN1 := valve_register, IN2 := 0, ENO => EQ21_ENO);
  EQ36_OUT := EQ(EN := TRUE, IN1 := mode_register, IN2 := 2, ENO => EQ36_ENO);
  LE48_OUT := LE(EN := EQ36_OUT, IN1 := Level, IN2 := min_sp, ENO => LE48_ENO);
  IF EQ21_OUT OR LE48_OUT THEN
    valve := FALSE; (*reset*)
  END_IF;
END_PROGRAM


CONFIGURATION Config0

  RESOURCE Res0 ON PLC
    TASK TaskMain(INTERVAL := T#50ms,PRIORITY := 0);
    PROGRAM Inst0 WITH TaskMain : My_Program;
  END_RESOURCE
END_CONFIGURATION