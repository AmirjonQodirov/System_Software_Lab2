<?php

$ffi = FFI::cdef( file_get_contents('./lib/lib.h'),__DIR__ . '/lib/lib.so');
$welc = 'Welcome!' . PHP_EOL .
        '-show partitions (p)' . PHP_EOL .
        '-work with fs (fs)' . PHP_EOL;
print($welc);
$a = readline();
if($a == "p"){
    $arr1 = $ffi-> show_parts();
    $string1 = FFI::string($arr1);
    print($string1);
}elseif($a == "fs"){
    $dest = readline('Put destination to hfsp fs: ');
    $arr2 = $ffi->read_hfsp($dest);
    $string2 = FFI::string($arr2);
    print($string2);
    print(PHP_EOL);
    $line = readline('command: ');
    $command = explode(" ", $line);
    while($command[0] != 'exit'){
        if($command[0] == 'pwd'){
            $arr3 = $ffi->show_pwd();
            $string3 = FFI::string($arr3);
            print($string3);
            print(PHP_EOL);
        }elseif($command[0] == 'ls'){
            $arr3 = $ffi->show_ls();
            $string3 = FFI::string($arr3);
            print($string3);
            print(PHP_EOL);
        }elseif($command[0] == 'cd'){
            $dest = $command[1];
            $arr3 = $ffi->do_cd($dest);
            $string3 = FFI::string($arr3);
            print($string3);
            print(PHP_EOL);
        }elseif($command[0] == 'back'){
            $arr3 = $ffi->do_back();
            $string3 = FFI::string($arr3);
            print($string3);
            print(PHP_EOL);
        }elseif($command[0] == 'copy'){
            $file = $command[1];
            $dest = $command[2];
            $arr3 = $ffi->do_copy($file,$dest);
            $string3 = FFI::string($arr3);
            print($string3);
            print(PHP_EOL);
        }else{
            print('wrong command');
            print(PHP_EOL);
        }
        $line = readline('command: ');
        $command = explode(" ", $line);
    }
}


?>

