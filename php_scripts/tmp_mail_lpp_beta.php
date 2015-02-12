<?

// This script is used for mail sending service provided by lpp-3ds 
// By Rinnegatamante

function containsInjectionAttempt($input) {
if (eregi("\r", $input) or
eregi("\n", $input) or
eregi("%0a", $input) or
eregi("%0d", $input) or
eregi("Content-Type:", $input) or
eregi("bcc:", $input) or
eregi("to:", $input) or
eregi("cc:", $input)) {
return true;
} else {
return false;
}
} 
if ((containsInjectionAttempt($_GET['t'])) or (containsInjectionAttempt($_GET['b'])) or (containsInjectionAttempt($_GET['s']))){
	echo 0;
}else{
	if (mail($_GET['t'] ,$_GET['s'] ,$_GET['b'])){
		echo 1;
	}else{
		echo 0;
	}
}
?>