# asciiplay

Renders images as ASCII art and plays video as animated ASCII art in the
terminal, in colour, at the original frame rate and with sound.

## What it looks like

`sample.png` at 100 columns, with `--glyphs braille --cell 2x4 --dither --color none`:

```text
⠀⢀⠀⢀⠀⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠀⢀⠀⡀⠀
⢐⠠⠐⠠⢀⢁⠠⢀⢁⠈⠠⠈⡀⠐⠀⠂⡀⠂⠀⠄⠠⠀⠠⠀⡀⢀⠀⡀⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡀⠀⡀⠀⡀⢀⠠⠀⠠⠀⠄⠠⠐⠀⠐⠀⠂⡀⠁⠄⢁⠈⡀⠡⢀⠐⡀⠄⡐⠠
⡐⢌⠌⡌⡂⡂⡂⡂⡢⠨⢐⢁⠄⠅⠌⠄⡂⠨⢀⢂⢐⠈⠠⢀⠄⠠⠀⠄⠠⠀⠂⢁⠈⡀⢁⠀⡁⠄⠁⠠⠈⢀⠐⠀⠂⠐⠀⠂⠐⠀⠂⠐⠀⠂⠐⠀⠂⠐⠈⠀⠄⠁⡀⢁⠀⡁⢈⠀⡁⠄⠠⠀⠄⠂⠠⠀⠄⡐⢀⢂⢐⢀⠂⠌⠄⠅⡂⡂⠅⢌⢐⢐⠨⡐⢄⢅⠢⡑⡨⠨
⢌⢆⠕⡔⡑⡌⡢⡑⢔⠅⡕⡰⠨⡊⡌⡪⢐⠅⡅⡢⢂⠅⢕⢐⠌⢌⠌⢌⢂⢅⢑⢐⢐⢐⢐⠐⢄⠂⢅⠂⢅⠐⠄⠅⠌⠄⠅⠌⠄⠅⠌⠄⠅⠌⠄⠅⠌⠄⠅⠌⠄⢅⢐⢐⢐⢐⢐⢐⢐⠨⡐⡡⠊⢌⠌⢌⠢⡨⢂⢂⠆⡢⢡⢑⠅⢕⠰⡨⡘⡄⢕⠰⡑⢌⢢⠢⡱⢨⢂⢇
⡪⡢⡣⡱⡱⡸⡰⡑⡅⡇⡪⡸⡨⡢⡱⡨⡢⡃⡎⢔⢅⢕⠱⡐⠥⡑⡌⢆⢕⢐⢅⢪⢐⠅⡆⡣⡑⢌⢢⢑⠅⢕⠡⠣⡑⢅⢣⢑⢅⠣⡑⢅⠣⡑⡅⡣⡑⡅⡣⡑⡅⢕⡐⡅⢆⢕⠰⡡⠢⡃⢆⢪⢘⠔⡅⢕⢅⠪⡢⡡⡱⡘⡔⢅⢕⢅⢇⢪⠢⡪⡊⡎⡪⡊⡆⡇⡕⡕⡱⡨
⡪⡪⡪⡪⡪⡪⡪⡪⡪⡪⡪⡪⡪⡪⡪⡪⡢⡣⡪⡪⡢⡣⡣⡣⡫⡊⡎⡪⡢⡣⡱⡸⡰⡱⡑⡌⡎⡪⡢⡣⡱⡑⡍⡎⡪⡊⡆⡕⡜⡌⡎⡪⡊⡎⡢⡱⡸⡐⡕⡜⡌⡖⢜⢌⢆⡣⡱⢌⢇⢣⢣⢱⢡⢣⢪⢪⢢⢣⢣⢱⢱⢱⢸⢸⢸⢰⢱⢱⢱⢱⢱⢱⢱⢱⢱⢱⢱⡱⡱⡕
⡇⡯⡪⣇⢗⡕⡧⡳⡱⣝⢜⡎⡮⡪⣎⢮⢪⢎⢞⡜⣎⢮⢪⡪⣪⡪⡎⡮⡪⡪⡪⡪⠪⡪⢪⢪⡪⣪⢪⢪⢪⡪⡪⣪⢪⡪⣪⢪⢪⢪⢪⡪⡪⣪⢪⡪⡪⡪⡪⡪⡪⡪⣪⢪⡪⡪⡪⡣⣣⢣⡣⡇⡧⡣⡇⡧⡣⡇⡧⡳⡱⡕⣕⢇⢗⡕⣇⢗⡕⡧⡳⡕⣇⢯⡪⡇⣗⢕⢧⢫
⡳⡝⣞⢼⢕⣝⢎⡯⡺⣪⣣⡳⣝⢮⡺⣜⢵⡹⣕⣕⢧⡳⣕⢝⡲⡱⡱⡱⡱⡱⡱⡨⡣⡊⡢⢂⢊⠘⢪⢳⢕⢧⡫⢮⣪⢺⢜⢎⡗⣝⡜⣎⢧⢳⢕⢵⢝⢎⡗⣝⢎⢧⡳⣕⢵⢝⢎⡗⣕⢧⢳⡹⣜⢮⢳⡹⣜⢮⢳⡹⣪⢳⢕⣝⢵⢕⢧⡳⡵⡝⡮⣺⣪⡺⣜⢞⣎⢗⢽⡱
⡽⣪⢗⡽⣕⣗⢽⡪⡯⣺⢜⣞⢮⡳⣕⡗⣗⣝⢮⡺⣪⢞⢎⢕⡕⡵⣹⣜⡮⣪⢪⢊⢆⢪⠨⡂⡢⠁⡀⠈⠳⡵⣝⢵⡣⡯⣺⢵⢝⢮⣺⣪⡳⣝⢮⡳⣝⢵⢝⢮⡫⡧⣳⢵⢝⡮⣳⢝⢮⡳⡳⣝⢮⡺⡵⣝⢮⡺⡵⣝⢮⡳⣝⢮⡳⣝⡵⣝⢮⡫⣞⢵⡺⣺⣪⡳⣕⢯⡳⣝
⣯⡳⡯⡯⣞⡮⣗⡯⣯⡳⡯⣞⡵⡯⣞⢾⢵⡳⡯⣞⡗⡕⡕⣕⢵⣻⣽⣾⡿⣕⢇⢇⠪⡂⢕⠐⠄⡁⠠⠀⠀⠱⢽⢵⣫⢯⣞⡵⣯⣳⡳⣕⡯⣞⣵⣻⣪⢯⢯⣳⢽⣺⢵⡫⡳⡹⡘⢍⡑⠍⢍⠪⠳⢝⣗⡽⣵⣫⢯⣺⢵⡻⡮⣗⡯⣞⢾⢵⣫⢯⢾⣝⢾⣕⢷⢽⢽⢵⢯⣗
⣗⣯⢯⡯⣗⣯⣗⣟⣮⢯⡯⣗⣯⢯⡯⣯⢷⢯⡯⡷⣱⢱⢱⢱⢱⢝⡽⣫⢟⢎⢇⢕⢑⠌⡂⠅⡁⠄⠠⠀⠀⢀⢹⢽⣺⣳⣳⣻⣺⢮⡯⣗⣯⣗⣗⡷⣽⣝⣗⣯⡻⡎⢇⢣⢱⢸⢨⢒⠜⢌⠢⢊⠄⠂⠈⠻⣺⣺⣽⣺⡽⣽⢽⣺⡽⣽⢽⣽⣺⣽⣳⣽⣳⢯⢯⡯⣯⢯⣗⡷
⣟⡾⣯⢿⣽⣞⣷⣻⢾⣻⣽⢯⡿⣽⢯⡿⣽⢯⡿⣝⢮⢺⡸⡜⣜⢜⡜⣜⢜⢌⢆⢆⢅⢆⠢⡡⠠⠠⠠⠀⡈⡠⢐⢿⣽⣞⣿⣺⡽⣯⢿⡽⣞⡷⣯⣟⣷⣻⣞⡷⡹⡘⡜⣜⣼⣺⣼⢜⢜⢐⠅⢅⠂⡁⠐⠀⠉⡷⣷⢯⡿⣽⢯⡷⣿⢽⣻⣞⣷⣻⢾⣺⡽⣯⢿⡽⣯⣟⣷⣻
⣯⢿⣽⣻⣞⣷⣻⢾⣻⡽⡾⡯⡿⡽⡯⡿⡽⡯⣟⡧⡫⡣⡯⡺⣪⢳⡹⡜⣎⢗⢕⢇⢇⡇⠇⡣⡙⢜⠈⠂⠐⡈⠢⣻⣳⣻⣺⣳⢟⡯⡿⡽⡯⣟⢷⣻⣺⣳⣻⢪⢪⢸⠸⡼⣾⣿⢿⢕⢕⠡⡡⢁⠂⠄⠂⠀⡀⠸⡯⡿⡽⡯⡿⡽⡯⣟⣷⣻⢾⢽⢯⢿⢽⢯⢿⡽⣗⣿⣺⣽
⣗⣟⣞⣞⣞⢞⣞⢽⢮⢯⢯⢯⢯⢯⢯⢯⢯⢯⣳⢽⢸⢱⢣⢫⡪⣺⡸⡸⢪⢪⡊⡎⡎⠨⠐⠐⠈⠂⢄⠠⠐⡀⣡⣷⣳⣳⡳⣝⢽⡹⣝⢽⢝⣞⢽⡺⡺⣺⢪⡣⡣⢪⢣⡳⡕⡗⡝⡌⡆⡕⡐⡔⡐⢄⢂⠄⠄⡂⢯⢯⡫⡯⣫⢯⣫⢗⣗⢽⢝⡽⡽⡽⡽⡽⡽⣺⡳⣳⡳⣳
⣮⣖⢵⢕⢗⡻⡺⡳⡯⣮⣗⡽⡭⡳⣛⠮⡗⡷⡵⣽⣪⢪⢈⡇⠅⢢⢽⢔⡥⡆⠋⠊⠈⠂⢇⢇⠇⠈⠈⣢⣶⣿⣿⣯⢿⡾⣽⡺⣕⢝⢮⣳⡫⣮⡳⣝⢽⢪⠧⡃⢎⢪⢣⢣⢑⢕⠝⡜⡜⢜⠸⠨⡊⠌⢂⠁⠄⠀⣗⢗⢽⡹⣪⡳⣕⡯⡮⡗⡟⡞⡝⣎⢯⣪⢯⡺⡮⣗⢯⢳
⡗⡯⡿⣽⡵⡯⣎⣗⣝⢜⢎⢏⢯⢿⢽⢽⢵⡷⣝⣜⣜⣢⡑⢝⠜⠘⠪⠯⡺⣑⠁⠀⠠⡀⡄⣂⠀⢠⣾⣿⣿⣿⣿⣿⣯⢯⣗⢽⢜⢜⠄⠢⠷⡵⡵⡳⣝⢝⣝⡌⡢⣅⡓⡑⡁⡢⢣⢃⠨⠁⢑⠕⠔⠀⠄⠄⠐⡨⡮⡯⢷⢝⢝⢝⣪⣪⣚⢞⢮⢯⢽⡺⡽⡪⡯⡺⡱⣱⡱⣳
⡹⡩⣫⣫⣽⡽⣿⣺⣞⣾⡺⡺⡪⣒⢕⢝⢝⢹⣩⣳⣽⢷⣵⣦⣁⠀⠀⠀⠀⠀⢎⢇⢇⠇⠇⠃⢑⣽⣿⣻⣿⣿⣿⣿⣺⡳⣕⠧⡣⡃⠂⡈⢨⢶⡵⣗⣮⢯⢣⢫⢐⢜⢔⠄⡨⢜⢎⢎⢪⠸⡐⠀⠀⠀⠀⠁⡠⡋⢎⢪⠺⡸⡪⡮⣖⢷⢯⢯⣫⡫⡫⡚⡜⡪⡪⡺⢜⢶⢽⢮
⣪⣊⡆⡕⡴⢵⢵⢾⠾⡯⡿⢿⣻⣟⣝⣍⣇⣕⣔⣑⣌⡪⡰⡲⡷⢟⢔⢄⠄⡀⢈⢀⢁⡀⡈⡄⡸⣯⣿⣽⢯⣟⡾⣺⢜⢮⡪⡪⡒⢌⢐⠐⢀⢕⣌⢆⣪⣘⢜⡯⠿⡧⡄⠀⠘⠨⢂⠣⡡⡑⠌⠀⠀⢀⢄⢕⡑⡝⡜⢎⢇⠗⡝⢜⢔⠬⡢⢱⣐⢕⣌⣪⣩⡹⣹⡺⡿⡽⢯⠿
⣍⢪⡨⣊⢌⢆⢕⢌⣮⢶⣯⢿⣯⢿⡯⣿⢷⣿⢝⢏⢕⡑⣌⢢⣊⢢⣑⡐⡅⣮⢾⢾⣳⢷⣟⡾⣽⣻⣺⢽⢯⢗⡯⣫⢫⢣⢣⠣⠑⠄⠂⢈⠰⣿⡻⣟⣯⢿⣻⡽⣎⢰⢨⢊⢄⢄⣀⡀⣀⡀⢄⢲⢸⢱⢱⢱⢱⢱⢹⢔⢆⣕⠨⣂⢢⡑⣌⢢⢊⢍⣚⢳⡳⡿⡯⣟⡯⡿⡷⡷
⣿⢿⠻⡛⢝⠡⡃⢕⢐⠅⡢⢑⠄⣕⣬⣶⣿⣾⣿⣿⣿⣿⢿⣟⣿⡟⡗⠍⢌⢂⠅⢕⠠⡡⢂⠌⢔⢸⡺⡕⡧⣳⣫⢺⢸⢘⠄⠀⢀⠠⠩⠀⠌⡐⠨⢐⢐⠐⠔⡨⢐⠹⣻⣟⣿⣻⣾⣻⣷⢿⣟⣿⣳⣴⡐⡡⠂⠕⡐⢌⢂⠢⡑⠩⡙⠽⢯⣟⣿⣽⢾⣯⣟⣯⣷⡶⣎⣆⡪⡐
⣮⣦⣧⣮⣦⣧⣮⣶⣦⢧⠎⡒⡛⠝⡫⢓⠛⠝⠝⠝⡓⡛⣛⣻⣽⣴⣼⣼⣴⣦⣧⣵⣼⣴⣵⣼⡴⢑⠣⠁⠑⠑⠕⠱⢡⠱⠨⠂⠂⠁⡄⡥⢢⢢⢡⢢⢰⢨⢰⢰⢰⣵⣼⡫⢋⢛⠚⢝⢙⠫⡋⠫⠛⡚⡙⠢⠣⣧⡮⣦⣦⡧⣮⣼⢴⡵⣦⡮⣗⣯⡫⢓⢋⠏⡚⠝⢝⠹⡙⠝
⣿⣿⣿⣿⣿⠿⡛⠍⢌⠢⠨⢂⠌⡂⡢⢁⠪⠨⠨⢂⣢⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⣿⣻⣿⠏⡂⠅⠌⠌⡂⠠⠀⠈⠀⠀⠀⡀⠠⠀⢇⢎⢎⢆⢇⢣⢱⣡⣣⣷⣻⣽⣿⣽⣆⠢⢑⠐⠄⠅⡂⠅⠅⡂⠌⠌⢌⢐⠙⠿⣞⣿⣻⡾⣟⣿⡽⣟⣯⣷⢿⡷⣧⣆⡪⡈⠢⠨⡐⠡
⢿⢿⣫⣣⣅⣪⣠⣥⣡⣬⣨⣰⣨⣰⣠⣅⣬⡨⠼⠾⠿⢟⠿⢟⠿⡻⢟⠿⡻⢟⠿⠿⡻⢟⣇⣅⣢⣡⣡⣡⣢⣡⣌⣤⣡⣨⣠⣠⣐⣌⠮⠮⠶⠷⠷⠿⠻⠯⢟⠯⠿⡻⠯⠿⠻⠧⣢⣡⣡⣡⣢⣡⣂⣢⣡⣡⣢⣐⣌⣤⣑⣝⡯⢟⠟⠷⡻⠿⠽⠾⢻⠻⠽⡫⢟⠯⠷⠥⢨⣨
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⠫⠃⡂⠅⠌⡐⡐⠌⡐⡐⡐⡐⠌⡐⡐⠨⢐⣰⣿⣿⣿⣿⣿⣿⣿⣿⣻⣿⣻⣿⣻⣿⡿⣿⣿⠠⠡⠈⠄⢂⠨⠀⠅⢐⠨⠐⠠⢁⢊⠨⢐⠈⢿⣟⣿⣽⣟⣿⣟⣿⣽⣯⣿⣽⣾⢿⣽⣟⣷⣬⡐⡀⠢⢈⢐⠐⠨⠐⡐⢐⢈⢐⠨⢀⠂
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣯⣿⡿⢏⠋⡐⢈⢐⢀⠊⠠⠂⡂⠅⢂⠂⡂⡂⠅⡂⠌⡨⣴⣿⣿⣿⣻⣽⣿⣿⣿⣻⣿⣿⣿⣿⣿⡿⣿⡿⣿⠐⢈⠐⢈⠀⡐⠈⡀⠂⠄⠡⢁⠂⠔⡈⠄⠌⡀⢛⣿⣽⣿⣽⣯⣿⢷⣟⣷⣿⣽⢿⡷⣿⣽⣾⢿⣲⣅⡐⠠⠈⠄⠡⠐⡀⢂⠐⡈⠄⠌
⣿⣿⣿⣿⣿⣿⣿⣿⣿⠻⢙⠈⠄⠂⢂⠂⡐⠠⠈⠄⠡⠐⡈⡐⢐⢀⢂⠂⡂⣡⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣻⣷⣿⣿⣿⣿⠐⢀⠂⠄⠂⡀⠂⡐⠈⡀⠅⢐⠈⡐⡐⠡⢁⠐⠠⠘⢿⣾⣯⣷⣿⢿⣻⣽⣾⣟⣿⣻⣯⣷⡿⣟⣿⣯⣿⣦⣅⠌⠠⢁⠐⡀⢂⠐⢐⠠
⠛⠛⠛⢛⢙⠫⣛⣷⣵⣼⣴⣴⣵⣼⣴⣴⣦⣮⣦⣵⣬⣦⣶⣴⣵⣴⣦⣮⡆⡛⠫⠛⢋⠛⡙⠝⠋⡛⠫⠛⡋⡛⠝⢋⠛⠛⡙⢋⠫⠛⣴⣦⣦⣶⣴⣴⣦⣦⣦⣦⣦⣦⣶⣴⣴⣵⣴⣬⣦⣮⣦⣟⠙⠝⠚⠛⠋⠛⡙⢙⠙⡙⠙⠓⠛⠋⠛⢊⠓⢋⠋⡓⠑⢴⣴⣴⣴⣬⣦⣦
⠈⠌⡈⢄⣴⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠫⠐⡀⠅⠈⠄⠂⡀⠂⡁⠄⠅⡂⠢⢐⢐⠠⠈⠄⠐⡀⠄⠨⣿⣿⣻⣽⣿⣯⣿⣯⣿⣟⣿⣯⣿⣟⣿⣟⣿⣿⢿⣿⣻⣯⣧⡐⠈⢀⠁⡐⠀⠄⠐⠀⡁⢈⠀⠡⠈⠠⠐⢀⠐⢀⠁⠄⠌⠫⢷⡿⣷⢿
```

That is the monochrome rendering — every dot is one pixel of a 200×112 grid, packed
two across and four down into each character cell. Drop the `--color none` and each
dot is tinted with the colour of the pixel underneath it.

The sample is raytraced rather than photographed, so the repository carries no
third-party imagery and you can reproduce it byte for byte:

```sh
pip install numpy pillow
python3 tools/gen_sample.py -o sample.png
```

## How it works

Most ASCII-art converters map each block of the image to a character by average
brightness, using a ramp like `" .:-=+*#%@"`. That throws away all structure —
an edge and a flat grey patch of the same mean brightness get the same
character.

`asciiplay` does **shape matching** instead. Every candidate glyph is rasterised
from a real monospace font into an 8×16 ink-coverage map (baked into
`src/glyph_table.h`). For each block of the source image it picks the glyph
whose ink pattern best fits, then colours the glyph with the mean colour under
its ink and leaves the cell background as your terminal's own — the picture is
made of coloured glyph shapes, not tinted rectangles.

`--bg` opts into the two-colour cell instead: the background is filled with the
mean colour *behind* the ink, so every cell carries two colours. That packs more
colour detail in, but at small cell counts it reads as a mosaic of blocks rather
than as drawn shapes.

Matching reduces to one dot product per candidate, and which criterion applies
follows from that choice:

- **ink-only** (the default, and everything under `--color none`) minimises
  `‖block − glyph‖²` against a fixed white-on-black rendering. The glyph alone
  has to carry the tone, so absolute ink is what matters: bright blocks fill,
  dark blocks empty.
- **`--bg`** maximises the correlation between the block and the glyph. Because
  the stored glyph rows are mean-centred, the block's own brightness cancels
  out — the two cell colours supply it — so the match is invariant to local
  brightness and contrast and only ranks shape.

Decoding is delegated to `ffmpeg` over a pipe, which also does the scaling. That
means anything ffmpeg can read works, and there is nothing to link against.

## Requirements

- A C++17 compiler and CMake ≥ 3.16
- `ffmpeg` and `ffprobe` on `PATH`; `ffplay` too if you want sound

| Platform | Install ffmpeg |
|---|---|
| Debian / Raspberry Pi OS | `sudo apt install ffmpeg` |
| macOS | `brew install ffmpeg` |
| Windows | `winget install Gyan.FFmpeg` |

On Windows you need Windows 10 1511 or newer for ANSI colour support. Use
**Windows Terminal**, not the legacy console host — it is far faster at the
volume of escape sequences video produces, and it renders the block glyphs
correctly.

## Build

```sh
cmake -S . -B build
cmake --build build --config Release -j
```

The binary lands at `build/asciiplay` (`build/Release/asciiplay.exe` on MSVC).

For Visual Studio or Xcode project files:

```sh
cmake -S . -B build -G "Visual Studio 17 2022"     # Windows
cmake -S . -B build -G Xcode                       # macOS
```

`-DASCIIPLAY_NATIVE=OFF` disables `-march=native` / `-mcpu=native`, which you
want if the binary has to run on a different machine than the one that built it.

## Usage

```sh
asciiplay photo.jpg                  # print a still, sized to your terminal
asciiplay clip.mp4                   # play with sound, q to quit
asciiplay clip.mp4 -c 160 --cell 4x8 # bigger, and fast enough to keep up
asciiplay photo.png -o art.txt       # write to a file (ANSI colour included)
asciiplay photo.png --color none     # plain ASCII, no escape codes
asciiplay photo.jpg --glyphs braille --cell 2x4 --dither   # coloured dots
asciiplay photo.jpg --bg             # two colours per cell instead
```

| Option | |
|---|---|
| `-c, --cols N` | width in characters (default: fit terminal) |
| `-r, --rows N` | height in characters |
| `--cell WxH` | match resolution per cell, default `8x16`; `4x8` is ~4× faster |
| `--ascii` | shortcut for classic ASCII art: `--glyphs ascii --color none --gamma 1.4` |
| `--glyphs SET` | any mix of `ascii`, `blocks`, `braille` joined by `+` (default `ascii+blocks`) |
| `--color MODE` | `true`, `256` or `none` (default `true`) |
| `--color-tol N` | merge neighbouring colours within `N` per channel into one escape (default `4`); the lever for terminal-bound playback |
| `--bg` / `--no-bg` | fill the cell background with a second colour (default off: only the glyph ink is coloured) |
| `--gamma G` | tone curve; `>1` darkens midtones |
| `--eq` / `--no-eq` | histogram equalisation (on by default unless `--bg` is given) |
| `--invert` | for light-background terminals |
| `--dither` | Floyd-Steinberg error diffusion; pair with `--glyphs braille --cell 2x4` |
| `--fps N` | override the frame rate |
| `--no-audio` | do not spawn ffplay |
| `--loop` | repeat until quit |
| `-j, --threads N` | matcher threads (default: all cores) |
| `-o, --out FILE` | write to a file instead of playing |
| `--image` / `--video` | override input type detection |

During playback: `q` or `Esc` quits, `space` pauses. Resizing the terminal
re-fits the picture and resumes from the same position.

## Choosing a glyph set

Measured on a 720×1280 video frame at 100 columns with `--bg`, as PSNR of the
reconstructed image against the original — higher is better (the two-colour cell
is what makes a pixelwise measure meaningful here; the default ink-only look
trades reconstruction error for legible shapes):

| `--glyphs` | PSNR |
|---|---|
| `ascii` | 23.3 dB |
| `blocks` | 26.6 dB |
| `ascii+blocks` (default) | 26.6 dB |

`braille` is a fourth option: 2x4 subpixels per cell, the densest grid a
character can carry. Its subpixels are strictly on/off, so a flat area snaps to
all-dots or none unless you add `--dither`, which trades spatial noise for
apparent tone. The combination to use is
`--glyphs braille --cell 2x4 --dither`, which with the default ink-only
colouring gives a screen of individually coloured dots at 2×4 the character
resolution.

Block elements are simply better reconstruction primitives than letterforms, so
they win most cells. `ascii+blocks` never loses to `blocks` and sometimes gains,
which is why it is the default. Use `--glyphs ascii` if you specifically want
output that is pure ASCII — text you can paste anywhere.

## Performance

The matcher is the hot loop: `cols × rows × glyphs × cellW × cellH` multiply-adds
per frame, threaded across cores. On a Raspberry Pi 5 (4 cores), playing a
30 fps clip:

| Size | `--cell 8x16` | `--cell 4x8` |
|---|---|---|
| 80 cols | 97% of frames shown | — |
| 120 cols | 89% | — |
| 160 cols | 42% | 97% |
| 200 cols | — | 86% |

If frames are being dropped, `--cell 4x8` is the first thing to reach for: it
quarters the matching cost for a small quality loss. Fewer columns is the other
lever.

### When the terminal is the bottleneck

Those numbers exclude the terminal emulator, and on a fast machine the terminal
is usually the real limit. A 199×49 braille clip on an M1 Pro uses under half of
one core and drops nothing when stdout goes to `/dev/null` — but the same run
can crawl in a slow terminal, purely on the volume of escape sequences it has to
parse. macOS Terminal.app is the usual culprit; iTerm2, kitty, Ghostty and
WezTerm are all far quicker.

Per frame at 199×49 braille, before and after the escape-stream work:

| Mode | Was | Now |
|---|---|---|
| `--color none` | 30 KB | 30 KB |
| `--color 256` | 153 KB | **48 KB** |
| `--color true` | 209 KB | **87 KB** |

Three things get that: the background is reset once per frame rather than
restated on every cell, `--color 256` merges runs by palette slot instead of by
raw RGB (three quarters of its escapes were re-sending a colour the terminal
already had), and `--color-tol` merges neighbours whose colours differ by less
than the eye can tell. The on-screen error stays within the tolerance — it is
measured against the colour the terminal is actually holding, and re-anchored
every frame, so it cannot drift.

If playback is still choppy, in order of effect:

1. `--color 256` — a third of truecolour's bytes, and at this cell size hard to
   tell apart.
2. `--color-tol 8` or `16` — 67 KB and 54 KB per frame respectively. For
   comparison, the 256-colour cube quantises in steps of 40.
3. Fewer columns, or a faster terminal.

`--color none` is the floor: 30 KB/frame here is essentially just the glyphs
themselves, since a braille character is 3 bytes of UTF-8.

## Using a different font

The baked-in glyph shapes come from DejaVu Sans Mono. If your terminal uses a
noticeably different font, regenerate the table to match it (needs Python and
Pillow):

```sh
python3 tools/gen_glyphs.py /path/to/YourMono.ttf > src/glyph_table.h
cmake --build build
```

or via CMake:

```sh
cmake -S . -B build -DASCIIPLAY_FONT=/path/to/YourMono.ttf
cmake --build build --target glyphs
```

## Layout

```
CMakeLists.txt
src/main.cpp             argument parsing, still rendering, playback loop
src/asciiart.h/.cpp      glyph set, shape matcher, colour extraction, ANSI output
src/platform.h           OS abstraction: processes, terminal, key polling
src/platform_posix.cpp   Linux / macOS / BSD
src/platform_win.cpp     Windows
src/glyph_table.h        generated glyph coverage table
tools/gen_glyphs.py      regenerates the above from a TTF
```

Only the two `platform_*.cpp` files contain OS-specific code; each guards itself
with `#ifdef`, so both can sit in the build and only one compiles.
