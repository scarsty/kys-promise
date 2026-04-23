{$IFDEF android}
library kys_promise;

{$ELSE}
program kys_promise;
{$ENDIF}

//{$APPTYPE GUI}

uses
  SysUtils,
  LCLIntf,
  LCLType,
  LMessages,
  Forms,
  Interfaces,
  kys_main in 'kys_main.pas',
  kys_event in 'kys_event.pas',
  kys_battle in 'kys_battle.pas',
  kys_engine in 'kys_engine.pas',
  //kys_script in 'kys_script.pas',
  kys_littlegame in 'kys_littlegame.pas',
  Dialogs;

  //{$R kys_promise.res}


  {$R *.res}

  {$IFDEF android}
exports
  Run;
  {$ENDIF}

begin
  // Application.Title := 'KYS';
  // alpplication..Create(kysw).Enabled;
  // form1.Show;
  {$ifndef android}
  Application.Initialize;
  Run;
  {$endif}
end.
