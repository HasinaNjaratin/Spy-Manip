<?php

function myprint($variables, $die = TRUE){
	print '<pre>';
	print "*******------START--------*********";
	print_r($variables);
	if($die){
		die;
	}else{
		print "*******------BREAK--------*********";
	}
}