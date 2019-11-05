#!/bin/bash

hw=fraig
dofile=mydo
MyProg=./fraig
RefProg=./fraig-linux18

echo Begin running $dofile ...

$MyProg < $dofile &> om.log
echo $MyProg done
$RefProg < $dofile &> or.log
echo $RefProg done

colordiff om.log or.log

echo Finished running $dofile
