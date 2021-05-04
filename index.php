<?php
// создаём объект FFI, загружаем libc и экспортируем функцию printf()
$ffi = FFI::cdef( file_get_contents('./lib/lib.h'),__DIR__ . '/lib/lib.so');
// вызываем printf()

$my_string = './../../fs/myHFS.fs';


$array = $ffi->read_hfsp($my_string);

$arr2 = $ffi->show_ls();
$string1 = FFI::string($arr2);
print($string1);
?>

