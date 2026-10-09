# ftxui_repl_pipe_bom.ps1 - The FTXUI REPL drops the UTF-8 byte order mark
# that Windows PowerShell writes before the text it pipes into a program, on
# a one-line and on a two-line first command.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File ftxui_repl_pipe_bom.ps1 <repl_ftxui_async_fixture>
#
# Returns 0 if every check passes, non-zero otherwise.

param([Parameter(Mandatory = $true)][string]$Fixture)

# With this encoding Windows PowerShell writes the mark before the piped
# text, as it does for a user whose console is set to UTF-8.
$OutputEncoding = New-Object System.Text.UTF8Encoding $true
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false
$status = 0

function Check([string]$Name, [string[]]$Lines, [string]$Expected) {
	$out = @(($Lines -join "`n") | & $Fixture)
	$text = $out -join "`n"
	if ($text.Contains([string][char]0xFEFF)) {
		Write-Host "FAIL: ${Name}: the output keeps the byte order mark"
		$script:status = 1
	}
	if ($out -cnotcontains "evaluated: $Expected") {
		Write-Host "FAIL: ${Name}: no line 'evaluated: $Expected'"
		$script:status = 1
	}
	Write-Host $text
}

# Without the mark in the pipe the checks below prove nothing.
$hex = @("hello" | & $Fixture --hex) -join " "
Write-Host "piped bytes: $hex"
if (-not $hex.StartsWith("ef bb bf")) {
	Write-Host "FAIL: PowerShell wrote no byte order mark before the piped text: $hex"
	$status = 1
}

Check "one line" @("hello", "quit") "hello"
# the fixture asks for more lines after "incomplete", and then evaluates
# the whole buffer, the first line included
Check "two lines" @("incomplete", "more", "quit") "incomplete"

if ($status -eq 0) { Write-Host "PASS: ftxui_repl_pipe_bom" }
exit $status
