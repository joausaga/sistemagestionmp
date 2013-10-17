<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="<?php print $language->language ?>" lang="<?php print $language->language ?>">
<head>
<title>Sistema de Gestión | Movimiento Peregrino</title>
<?php print $head ?>
<?php print $styles ?>
<?php print $scripts ?>
</head>
<body>
<?php global $user; ?>
<div style="padding: 100px 0 0 390px;">
<div id="login-box">
<h2>Iniciar Sesión</h2>
Movimiento Peregrino - Sistema de Gestión
<br />
<br />
<form action="http://www.movimientoperegrino.org/sg/user?<?php print drupal_get_destination() ?>" method="post" id="user-login-form">
<div id="login-box-name" style="margin-top:20px;">Usuario:</div>
<div id="login-box-field" style="margin-top:20px;"><input type="text" maxlength="60" name="name" id="edit-name" size="30" value="" tabindex="1" class="form-login required" /></div>
<div id="login-box-name">Contraseña:</div>
<div id="login-box-field"><input type="password" name="pass" id="edit-pass" size="30" tabindex="2" class="form-login required" /></div>
<br />
<span class="login-box-options"><a href="http://www.movimientoperegrino.org/sg/user/password" title="Olvido su contrasena">Olvido su contraseña</a></span>
<br />
<br />
<span><input type="submit" name="op" id="edit-submit" value="" tabindex="3" class="login-box-submit" /></span>
<input type="hidden" name="form_id" id="edit-user-login" value="user_login" />
</form>
</div>
</div>
</body>
</head>
