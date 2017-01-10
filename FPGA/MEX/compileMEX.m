close all; clear all; clc;

mex -IC:\WinDriver\include -IP:\SVN\Driver\WD_Project TestReadWrite.c C:\WinDriver\lib\amd64\wdapi1160.lib P:\SVN\Driver\pcie_test\amd64\msdev_2013\Debug\vc709_libapi.lib;
