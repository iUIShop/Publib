#pragma once

#include <windows.h>

// 解压Zip文件，如果lpszTargetUnZipFolder为NULL或空，则解压到zip文件所在文件夹
// 在Win7上验证成功，XP上未测试，有网友说不成功。
// 本方法使用API SHFileOperation解压缩，解压速度很慢。对于包含大量小文件的压缩包，例如包含4000多个文件，压缩后700MB左右的
// 压缩包，解压可能需要10多分钟。但同一个压缩包，换成7-zip解压，只需要30多秒。
int UnZipFile(LPCTSTR lpszSourceZipFile, LPCTSTR lpszTargetUnZipFolder);
