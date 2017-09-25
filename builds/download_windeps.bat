powershell "$wc = New-Object System.Net.WebClient; $wc.DownloadFile(\"https://milek7.pl/.stuff/eu07exe/builddep4.zip\", \"%cd%\deps_win.zip\")"
powershell "$s = New-Object -ComObject shell.application; $z = $s.Namespace(\"%cd%\deps_win.zip\"); foreach ($i in $z.items()) { $s.Namespace(\"%cd%\").CopyHere($i) }"
