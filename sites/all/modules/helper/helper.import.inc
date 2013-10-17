<?php

/**
 * @file
 * Administration forms of person import in helper module.
 */

module_load_include('inc', 'node_import');
drupal_add_css(drupal_get_path('module', 'node_import') .'/node_import.css');

function set_content_type(&$form_state) {
    if (!isset($form_state['values']['type'])) {
        $form_state['storage']['type'] = 'node:person';
    }
}

function set_file_options(&$form_state) {
    // Auto detect file format if not set.
    $files = node_import_list_files();
    $file = $files[$form_state['storage']['fid']];

    if (!isset($form_state['storage']['file_options'])) {
        $form_state['storage']['file_options'] = node_import_autodetect($file->filepath);
    }
    if (!isset($form_state['storage']['has_headers'])) {
        $form_state['storage']['has_headers'] = 1;
    }

    // If the user has chosen a predefined format, make sure
    // all options are correct.
    if ($form_state['storage']['file_options']['file_format'] != '') {
        $file_formats = node_import_format_options('file formats');
        $form_state['storage']['file_options'] = array_merge($form_state['storage']['file_options'], $file_formats[$form_state['storage']['file_options']['file_format']]);
    }


    if (!isset($form_state['storage']['samples']) and !isset($form_state['storage']['headers'])) {
        // Read some sample data with current settings.
        $samples = array();

        $i = variable_get('node_import:number_of_samples', 5);
        $num_cols = 0;
        $file_offset = 0;
        $data = array();
        while ($i > 0 && is_array($data)) {
            $i--;
            list($file_offset, $data) = node_import_read_from_file($file->filepath, $file_offset, $form_state['storage']['file_options']);
            if (is_array($data)) {
                $samples[] = $data;
                $num_cols = max($num_cols, count($data));
            }
        }
        if (!$form_state['storage']['has_headers']) {
            $header = array();
            while ($num_cols > 0) {
                $header[] = t('Column @number', array('@number' => $num_cols));
                $num_cols--;
            }
            $header = array_reverse($header);
            array_unshift($samples, $header);
        }
        $form_state['storage']['samples'] = $samples;
        $form_state['storage']['headers'] = $samples[0]; //TODO: what happens if no data?
    }
}

function map_fields(&$form_state) {
    $fields = node_import_fields($storage['type']);
    foreach ($fields as $fieldname => $fieldinfo) {
        if (empty($fieldinfo['columns'])) {
            if ($fieldinfo['map_required'] && strlen($values['map'][$fieldname]) == 0) {
                form_set_error('map]['. $fieldname, t('!name field is required.', array('!name' => $fieldinfo['title'])));
            }
        }
        else {
            foreach ($fieldinfo['columns'] as $colname => $colinfo) {
                if ($colinfo['map_required'] && strlen($values['map'][$fieldname][$colname]) == 0) {
                    form_set_error('map]['. $fieldname .']['. $colname, t('!name field is required.', array('!name' => $colinfo['title'])));
                }
            }
        }
    }
    // Auto detect mapping if not set.
    if (!isset($form_state['storage']['map']) && isset($form_state['storage']['headers'])) {
        $form_state['storage']['map'] = node_import_automap($form_state['storage']['type'], $form_state['storage']['headers']);
    }
}

/**
 * Creates a new import task by letting the user fill in a wizard.
 */
function helper_import_add_form(&$form_state) {
    global $user;
    $form = array();

    //drupal_set_message(print_r($form_state));

    // Check if the user can create something.
    $types = node_import_types();
    if (empty($types)) {
    form_set_error('', t('No tiene permisos para crear contenidos. <a href="!permissions">Asigne correctamente los permisos</a> en la página de administración.', array('!permissions' => url('admin/user/permissions'))));
    }

    // ------------------------------------------------------------
    // Get the currently filled in values of the form.
    $form_state['storage'] = isset($form_state['storage']) ? $form_state['storage'] : array();
    $form_state['values'] = isset($form_state['values']) ? $form_state['values'] : array();
    $values = array_merge($form_state['storage'], $form_state['values']);

    // ------------------------------------------------------------
    // Check and store the page we are on.
    $pages = array(
    'file' => array(
        'title' => t('Select file'),
        'description' => t('Seleccione el archivo <a href="http://es.wikipedia.org/wiki/CSV" target="_blank">CSV</a> que contiene los datos a importar.<br>Un archivo CSV, es un archivo donde los datos se encuentran separados por coma. Cada dato contenido entre comas se denomina campo, el cual debe estar entre comillas dobles.<br>Puede descargar un ejemplo de archivo CSV <a href="http://localhost/nuevo_sistema_mp/sites/default/files/ejemplo.csv">aqui</a>.'),
        ),
    'preview' => array(
        'title' => t('Vista Previa'),
        'description' => t('Aquí pueda visualizar previamente los datos a importar. Si encuentra errores puede hacer volver %back o comenzar de nuevo. También puede recargar solo esta página por si ocurrió un error al cargarla %reload.', array('%reload' => t('Recargar'), '%back' => t('Atrás'))),
        ),
    'start' => array(
        'title' => t('Iniciar Importación'),
        'description' => t('Si todo está correcto, clic %start.', array('%start' => t('Start import'))),
        ),
    );

    $page_keys = array_keys($pages);
    $first_page = $page_keys[0];
    $last_page = $page_keys[count($page_keys) - 1];

    $page = isset($values['page']) ? $values['page'] : $first_page;
    $form['#pages'] = $pages;
    $form_state['storage']['page'] = $page;

    // ------------------------------------------------------------
    // Set title and description for the current page.
    $steps = count($pages);
    $step = array_search($page, $page_keys) + 1;

    if ($page !== 'intro') {
        drupal_set_title(t('@title (Paso @step de @steps)', array('@title' => $pages[$page]['title'], '@step' => $step, '@steps' => $steps)));
    }

    $form['help'] = array(
        '#value' => '<div class="help">'. $pages[$page]['description'] .'</div>',
        '#weight' => -50,
    );

    // ------------------------------------------------------------
    // Select or upload a file.
    if ($page == 'file') {
        $files = node_import_list_files(TRUE);

        $form['file_select'] = array(
            '#type' => 'item',
            '#title' => t('Seleccione el archivo'),
            '#theme' => 'node_import_file_select',
        );

        if (isset($values['fid'])) {
            foreach ($files as $fid => $file) {
                if ($fid == $values['fid']) {
                    $file_owner = user_load(array('uid' => $file->uid));
                    $form['file_select'][$fid] = array(
                      'filename' => array('#value' => $file->filename),
                      'filepath' => array('#value' => $file->filepath),
                      'filesize' => array('#value' => format_size($file->filesize)),
                      'timestamp' => array('#value' => format_date($file->timestamp, 'small')),
                      'uid' => array('#value' => ($file->uid == 0 ? t('Public FTPd file') : theme('username', $file_owner))),
                    );
                    $aux_files = node_import_extract_property($files, 'filename');            
                    $form['file_select']['fid'] = array(
                        '#type' => 'radios',
                        '#options' => $aux_files['fid'],
                        '#default_value' => isset($values['fid']) ? $values['fid'] : 0,
                    );
                }
            }
            set_content_type($form_state);
            set_file_options($form_state);
            map_fields($form_state);
        }
        else {
            $form['file_select']['fid'] = array(
                '#type' => 'item',
                '#value' => t('No files available.'),
            );
        }

        $form['#attributes'] = array('enctype' => 'multipart/form-data');

        $form['upload'] = array(
            '#type' => 'fieldset',
            '#title' => t('Upload file'),
            '#description' => t(''),
            '#collapsible' => FALSE,
            '#collapsed' => FALSE,
        );
        $form['upload']['file_upload'] = array(
            '#type' => 'file',
        );
        $form['upload']['file_upload_button'] = array(
            '#type' => 'submit',
            '#value' => t('Upload'),
            '#submit' => array('node_import_add_form_submit_upload_file'),
        );
    }

    // ------------------------------------------------------------
    // Preview.
    if ($page == 'preview') {
        $form['preview_count'] = array(
          '#type' => 'select',
          '#title' => t('Number of records to preview'),
          '#default_value' => isset($values['preview_count']) ? $values['preview_count'] : variable_get('node_import:preview_count', 5),
          '#options' => drupal_map_assoc(array(5, 10, 15, 25, 50, 100, 150, 200)),
        );

        $form['preview'] = array(
          '#title' => t('Previa'),
        );

        foreach ($values['previews'] as $i => $preview) {
          $form['preview'][] = array(
            '#type' => 'item',
            '#title' => t('Record @count', array('@count' => $i + 1)),
            '#value' => $preview,
          );
        }
    }

    // ------------------------------------------------------------
    // Start import.
    if ($page == 'start') {
        $files = node_import_list_files();
        $file = $files[$values['fid']];        
        $form[] = helper_import_task_details($values);
    }

    // ------------------------------------------------------------
    // Add Back, Next and/or Start buttons.
    $form['buttons-bottom'] = array(
        '#weight' => 50,
    );
    $form['buttons-bottom']['back_button'] = array(
        '#type' => 'submit',
        '#value' => t('Back'),
        '#submit' => array('node_import_add_form_submit_back'),
        '#disabled' => ($page == $first_page),
    );
    $form['buttons-bottom']['reload_button'] = array(
        '#type' => 'submit',
        '#value' => t('Reload page'),
        '#submit' => array('node_import_add_form_submit_reload'),
        '#disabled' => ($page == $first_page),
    );
    $form['buttons-bottom']['reset_button'] = array(
        '#type' => 'submit',
        '#value' => t('Reset page'),
        '#submit' => array('node_import_add_form_submit_reset'),
        '#disabled' => ($page == $first_page),
    ); 
    $form['buttons-bottom']['next_button'] = array(
        '#type' => 'submit',
        '#value' => ($page == $last_page) ? t('Start import') : t('Siguiente'),
        '#validate' => array('helper_import_add_form_validate_next'),
        '#submit' => array('helper_import_add_form_submit_next'),
        '#disabled' => empty($types),
    );    

    return $form;
}

function helper_import_add_form_validate_next($form, &$form_state) {
    $values = $form_state['values'];
    $storage = $form_state['storage'];
    $page = $storage['page'];

    if ($page == 'file') {
        // Validate whether a file has been chosen and the file exists.
        $files = node_import_list_files();
        if (!isset($values['fid'])) { // || !isset($files[$values['fid']])) {
            form_set_error('fid', t('%select-file field is required. Upload a file first if there are no files uploaded yet.', array('%select-file' => t('Select file'))));
        }
    }
}

function helper_import_add_form_submit_upload_file($form, &$form_state) {
    $validators = array();

    $dest = node_import_directory();

    if (($file = file_save_upload('file_upload', $validators, $dest, FILE_EXISTS_RENAME))) {
        drupal_set_message(t('New file %name uploaded to %path.', array('%name' => $file->filename, '%path' => $file->filepath)));
        file_set_status($file, FILE_STATUS_PERMANENT);
        $form_state['values']['fid'] = $file->fid;
    }

    helper_import_add_form_submit_reload($form, $form_state);
}

function helper_import_add_form_submit_next($form, &$form_state) {
  $page = $form_state['storage']['page'];
  $page_keys = array_keys((array)$form['#pages']);
  $last_page = $page_keys[count($page_keys) - 1];

  if ($page == $last_page) {
    // If we are on the last page, save the task and redirect to task page.
    $form_state['storage'] = array_merge((array)$form_state['storage'], (array)$form_state['values']);
    $taskid = node_import_save_task($form_state['storage']);
    drupal_set_message(t('New import task has been created.'));

    $form_state['rebuild'] = FALSE;
    $form_state['storage'] = array();

    $form_state['redirect'] = 'admin/content/node_import/'. $taskid;
    $form_state['taskid'] = $taskid;
  }
  else {
    // Otherwise process current input and go to next page.
    helper_import_add_form_submit_reload($form, $form_state);

    $form_state['storage']['page'] = $page_keys[array_search($form_state['storage']['page'], $page_keys) + 1];

    helper_import_add_form_submit_reload($form, $form_state);
  }
}

function helper_import_add_form_submit_back($form, &$form_state) {
  $page_keys = array_keys((array)$form['#pages']);
  $form_state['storage']['page'] = $page_keys[array_search($form_state['storage']['page'], $page_keys) - 1];

  helper_import_add_form_submit_reload($form, $form_state);
}

function helper_import_add_form_submit_reset($form, &$form_state) {
  $page = $form_state['storage']['page'];
  unset($form_state['values'][$page]);
  unset($form_state['storage'][$page]);
  helper_import_add_form_submit_reload($form, $form_state);
}

function helper_import_add_form_submit_reload($form, &$form_state) {
  $page = $form_state['storage']['page'];

  $form_state['storage'] = array_merge((array)$form_state['storage'], (array)$form_state['values']);
  $form_state['rebuild'] = TRUE;

  // ------------------------------------------------------------
  if ($page == 'preview') {
    $files = node_import_list_files();
    $file = $files[$form_state['storage']['fid']];

    if (!isset($form_state['storage']['preview_count'])) {
        $form_state['storage']['preview_count'] = variable_get('node_import:preview_count', 5);
    }

    $previews = array();
    $preview_count = $form_state['storage']['preview_count'];
    $has_headers = $form_state['storage']['has_headers'];

    $file_offset = 0;
    $data = array();
    if ($has_headers) {
        list($file_offset, $data) = node_import_read_from_file($file->filepath, $file_offset, $form_state['storage']['file_options']);
    }

    global $node_import_can_continue;
    $node_import_can_continue = TRUE;

    $form_state['storage']['defaults'] = isset($form_state['storage']['defaults']) ? $form_state['storage']['defaults'] : array();
    $form_state['storage']['options'] = isset($form_state['storage']['options']) ? $form_state['storage']['options'] : array();

    while ($preview_count > 0 && is_array($data) && $node_import_can_continue) {
        $preview_count--;
        list($file_offset, $data) = node_import_read_from_file($file->filepath, $file_offset, $form_state['storage']['file_options']);
        if (is_array($data)) {
            $previews[] = node_import_create($form_state['storage']['type'], $data, $form_state['storage']['map'], $form_state['storage']['defaults'], $form_state['storage']['options'], TRUE);
        }
    }

    if (!$node_import_can_continue) {
        $previews[] = t('It is impossible to show more previews as a new object has been created that can be referenced by other rows.');
    }

    $form_state['storage']['previews'] = $previews;
  }

  unset($form_state['values'][$page]);
}

function helper_import_task_details($values) {
  $form = array();

  if (isset($values['uid'])) {
    $user = user_load(array('uid' => $values['uid']));
    $form[] = array(
      '#type' => 'item',
      '#title' => t('Created by'),
      '#value' => theme('username', $user),
    );
  }

  if (isset($values['created'])) {
    $form[] = array(
      '#type' => 'item',
      '#title' => t('Created on'),
      '#value' => format_date($values['created']),
    );
    $form[] = array(
      '#type' => 'item',
      '#title' => t('Last updated on'),
      '#value' => format_date($values['changed']),
    );
  }

  if (isset($values['fid'])) {
    $files = node_import_list_files();
    $file = $files[$values['fid']];
    $form[] = array(
      '#type' => 'item',
      '#title' => t('Archivo'),
      '#value' => check_plain($file->filename),
      '#description' => check_plain($file->filepath) .' ('. format_size($file->filesize) .')',
    );
  }

  if (isset($values['file_options'])) {
    $rows = array();
    $rows[] = array(t('First row contains column names'), $values['has_headers'] ? t('Yes') : t('No'));
    foreach (array('record separator' => t('Record separator'), 'field separator' => t('Field separator'), 'text delimiter' => t('Text delimiter'), 'escape character' => t('Escape character')) as $key => $title) {
      if ($values['file_options'][$key] === '') {
        $rows[] = array($title, check_plain($values['file_options']['other '. $key]));
      }
      else {
        $stuff = node_import_format_options($key .'s');
        $rows[] = array($title, $stuff[$values['file_options'][$key]]);
      }
    }
    $form[] = array(
      '#type' => 'item',
      '#title' => t('File options'),
      '#value' => theme('table', NULL, $rows),
    );
  }

  if (isset($values['samples'])) {
    $form[] = array(
      '#type' => 'item',
      '#title' => t('Sample data'),
      '#value' => theme('node_import_sample_data', $values['samples'], variable_get('node_import:length_of_samples', 200)),
    );
  }

  if (isset($values['type'])) {
    $type_titles = node_import_extract_property(node_import_types(), 'title');
    $form[] = array(
      '#type' => 'item',
      '#title' => t('Tipo de Contenido'),
      '#value' => $type_titles[$values['type']],
    );
  }

  if (isset($values['map'])) {
    //TODO: can we construct some reasonable status for this?
  }

  if (isset($values['defaults'])) {
    //TODO: can we construct some reasonable status for this?
  }

  if (isset($values['options'])) {
    //TODO: can we construct some reasonable status for this?
  }

  return $form;
}

/**
 * View a task and its progress.
 */
function helper_import_view_form(&$form_state, $task) {
  drupal_set_title(t('Importing %name', array('%name' => $task['name'])));

  $form = array();
  $form['#task'] = $task;

  $form['status'] = array(
    '#type' => 'item',
    '#title' => t('Estado'),
    '#value' => $task['status'] == NODE_IMPORT_STATUS_DONE ? t('Completed') : t('In progress'),
  );

  if ($task['status'] == NODE_IMPORT_STATUS_DONE) {
    $form['row_done'] = array(
      '#type' => 'item',
      '#title' => t('Rows imported'),
      '#prefix' => '<div id="node_import-row_done">',
      '#suffix' => '</div>',
      '#value' => format_plural($task['row_done'], t('1 row'), t('@count rows')),
    );
    $form['row_error'] = array(
      '#type' => 'item',
      '#title' => t('Rows with errors'),
      '#prefix' => '<div id="node_import-row_error">',
      '#suffix' => '</div>',
      '#value' => format_plural($task['row_error'], t('1 row'), t('@count rows')),
    );

    $form['download'] = array(
      '#type' => 'fieldset',
      '#title' => t('Download rows'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );
    $form['download']['what_rows'] = array(
      '#type' => 'radios',
      '#title' => t('Rows to download'),
      '#options' => array(
        'all' => t('All rows'),
        'error' => t('Rows with errors'),
        'done' => t('Rows without errors'),
      ),
      '#default_value' => 'error',
    );
    $form['download']['what_cols'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Extra columns'),
      '#options' => array(
        'objid' => t('Object identifier'),
        'errors' => t('Errors'),
      ),
      '#default_value' => array('objid', 'errors'),
    );
    $form['download']['download_button'] = array(
      '#type' => 'submit',
      '#value' => t('Download'),
      '#submit' => array('helper_import_view_form_submit_download'),
    );
  }
  else {
    $form['progress'] = array(
      '#value' => '<div id="node_import-progress">'. t('You need to have Javascript enabled to see the progress of this import.') .'</div>',
    );

    drupal_add_js('misc/progress.js', 'core');
    $js_settings = array(
      'nodeImport' => array(
        'uri' => url('admin/content/node_import/'. $task['taskid']),
      ),
    );
    drupal_add_js($js_settings, 'setting');
    drupal_add_js(drupal_get_path('module', 'node_import') .'/node_import.js', 'module');
  }

  $form['details'] = array_merge(array(
    '#type' => 'fieldset',
    '#title' => t('Task details'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  ), helper_import_task_details($task));

  return $form;
}

function helper_import_view_form_submit_download($form, &$form_state) {
  $task = $form['#task'];

  $file_formats = node_import_format_options('file formats');
  $file_options = $task['file_options'];

  $mime = 'text/plain';
  if (isset($file_options['file_format']) && isset($file_formats[$file_options['file_format']]) && isset($file_formats[$file_options['file_format']]['mime'])) {
    $mime = $file_formats[$file_options['file_format']]['mime'];
  }

  $suffix = pathinfo($task['file']->filename, PATHINFO_EXTENSION);
  $filename = basename($task['file']->filename, '.'. $suffix);

  $sql = "SELECT * FROM {node_import_status} WHERE taskid = %d";

  switch ($form_state['values']['download']['what_rows']) {
    case 'error':
      $filename = $filename .'-reject.'. $suffix;
      $sql .= " AND status = ". NODE_IMPORT_STATUS_ERROR;
      break;

    case 'done':
      $filename = $filename .'-done.'. $suffix;
      $sql .= " AND status = ". NODE_IMPORT_STATUS_DONE;
      break;

    default:
    case 'all':
      $filename = $filename .'-all.'. $suffix;
      break;
  }

  $sql .= " ORDER BY file_offset ASC";

  header('Content-Type: '. $mime);
  header('Content-Disposition: attachment; filename='. $filename);

  $objid = in_array('objid', $form_state['values']['download']['what_cols']);
  $errors = in_array('errors', $form_state['values']['download']['what_cols']);

  if ($task['has_headers']) {
    $headers = $task['headers'];
    if ($objid) {
      array_unshift($headers, t('Object'));
    }
    if ($errors) {
      array_unshift($headers, t('Errors'));
    }
    print node_import_write_to_string($headers, $file_options);
  }

  $result = db_query($sql, $task['taskid']);
  while (($row = db_fetch_object($result))) {
    list($file_offset, $data) = node_import_read_from_file($task['file']->filepath, $row->file_offset, $file_options);
    if ($objid) {
      array_unshift($data, $row->objid);
    }
    if ($errors) {
      array_unshift($data, implode("\n", (array)unserialize($row->errors)));
    }
    print node_import_write_to_string($data, $file_options);
  }

  exit();
}

function helper_import_view_form_submit_delete($form, &$form_state) {
  $destination = '';
  if (isset($_REQUEST['destination'])) {
    $destination = drupal_get_destination();
    unset($_REQUEST['destination']);
  }
  $task = $form['#task'];
  $form_state['redirect'] = array('admin/content/node_import/'. $task['taskid'] .'/delete', $destination);
}

/**
 * Delete a task.
 */
function helper_import_delete_form(&$form_state, $task) {
  $form = array();

  if ($task['status'] != NODE_IMPORT_STATUS_DONE) {
    $form[] = array(
      '#type' => 'markup',
      '#value' => t('This import task cannnot be deleted as it has not finished yet.'),
    );

    return $form;
  }

  $form['taskid'] = array(
    '#type' => 'value',
    '#value' => $task['taskid'],
  );

  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $task['name'])),
    isset($_GET['destination']) ? $_GET['destination'] : 'admin/content/node_import/'. $task['taskid'],
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

function helper_import_delete_form_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    node_import_delete_task($form_state['values']['taskid']);
  }

  $form_state['redirect'] = 'admin/content/node_import';
}


/**
 * Theme the file selection list.
 */
function theme_helper_import_file_select($form) {
  $header = array('', t('Filename'), t('Uploaded on'), t('Uploaded by'));
  $rows = array();

  foreach (element_children($form) as $child) {
    if (is_numeric($child)) {
      $element = $form[$child];
      $file = array(
        '#type' => 'item',
        '#value' => $element['filename']['#value'],
        '#description' => $element['filepath']['#value'] .' ('. $element['filesize']['#value'] .')',
      );
      unset($form['fid'][$child]['#title']);
      $rows[] = array(
        drupal_render($form['fid'][$child]),
        drupal_render($file),
        drupal_render($form[$child]['timestamp']),
        drupal_render($form[$child]['uid']),
      );
      unset($form[$child]);
    }
  }

  if (empty($rows)) {
    $rows[] = array(
      'data' => array(
        array(
          'data' => t('No files available.'),
          'colspan' => count($header),
        ),
      ),
    );
    unset($form['fid']);
  }

  return theme('table', $header, $rows) . drupal_render($form);
}

/**
 * Theme the import task view list.
 */
function theme_helper_import_imports_list($form) {
  $header = array(t('Name'), t('Created on'), t('Created by'), t('Status'), array('colspan' => 2, 'data' => t('Operations')));
  $rows = array();

  foreach (element_children($form) as $child) {
    if (is_numeric($child)) {
      $rows[] = array(
        drupal_render($form[$child]['name']),
        drupal_render($form[$child]['created']),
        drupal_render($form[$child]['uid']),
        drupal_render($form[$child]['status']),
        drupal_render($form[$child]['view']),
        drupal_render($form[$child]['delete']),
      );
      unset($form[$child]);
    }
  }

  if (empty($rows)) {
    $rows[] = array(
      array(
        'data' => t('No imports pending. You can <a href="!node_import-add">create a new import</a> using the wizard.', array('!node_import-add' => url('admin/content/node_import/add'))),
        'colspan' => count($header) + 1,
      ),
    );
  }

  return theme('table', $header, $rows) . drupal_render($form);
}

/**
 * Theme the file format selection list.
 */
function theme_helper_import_file_format_select($form) {
  $header = array(t('File format'), t('Record separator'), t('Field separator'), t('Text delimiter'), t('Escape character'));
  $rows = array();

  $file_formats = $form['file_format']['#options'];
  foreach ($file_formats as $format => $title) {
    $rows[] = array(
      drupal_render($form['file_format'][$format]),
      isset($form[$format]['record separator']) ? drupal_render($form[$format]['record separator']) : drupal_render($form['record separator']) . drupal_render($form['other record separator']),
      isset($form[$format]['field separator']) ? drupal_render($form[$format]['field separator']) : drupal_render($form['field separator']) . drupal_render($form['other field separator']),
      isset($form[$format]['text delimiter']) ? drupal_render($form[$format]['text delimiter']) : drupal_render($form['text delimiter']) . drupal_render($form['other text delimiter']),
      isset($form[$format]['escape character']) ? drupal_render($form[$format]['escape character']) : drupal_render($form['escape character']) . drupal_render($form['other escape character']),
    );
    unset($form[$format]);
  }

  $form['file_format'] = array(
    '#type' => 'item',
    '#title' => $form['#title'],
    '#value' => theme('table', $header, $rows),
    '#description' => $form['#description'],
  );

  return drupal_render($form);
}

/**
 * Theme a table with sample data.
 */
function theme_helper_import_sample_data($data, $length = 0) {
  $header = NULL;

  foreach ((array)$data as $i => $fields) {
    foreach ((array)$fields as $j => $value) {
      if ($length > 0 && strlen($value) > $length) {
        $data[$i][$j] = check_plain(substr($value, 0, $length)) .' '. theme('placeholder', format_plural(strlen($value) - $length, t('(1 more character)'), t('(@count more characters)')));
      }
      else {
        $data[$i][$j] = check_plain($value);
      }
    }
  }

  if (!empty($data) > 0) {
    $header = array_shift($data);
  }
  else {
    $data = array(t('Empty data set.'));
  }

  return theme('table', $header, $data);
}

/**
 * Theme the mapping, defaults and options tables.
 */
function theme_helper_import_field_table($form) {
  $header = $form['#node_import-columns'];
  $rows = array();
  $groups = array();

  foreach (element_children($form) as $child) {
    if (!isset($form[$child]['#type']) || $form[$child]['#type'] != 'value') {
      $title = check_plain($form[$child]['#title']);
      $description = $form[$child]['#description'];
      $group = isset($form[$child]['#node_import-group']) ? $form[$child]['#node_import-group'] : '';
      unset($form[$child]['#title']);
      unset($form[$child]['#description']);

      if (!isset($groups[$group])) {
        $groups[$group] = array();
      }

      $groups[$group][] = array(
        check_plain($title) . '<div class="description">'. $description .'</div>',
        drupal_render($form[$child]),
      );
    }
  }

  if (isset($groups['']) && !empty($groups[''])) {
    $rows = array_merge($rows, $groups['']);
  }

  foreach ($groups as $group => $items) {
    if ($group !== '' && !empty($items)) {
      $rows[] = array(
        array('data' => $group, 'colspan' => 2, 'class' => 'region'),
      );
      $rows = array_merge($rows, $items);
    }
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => $form['#node_import-empty'], 'colspan' => 2));
  }

  return theme('table', $header, $rows) . drupal_render($form);
}

/**
 * Save a file with a debug report.
 */
function helper_import_debug_report($task) {
  drupal_set_header('Content-Type: text/plain');
  drupal_set_header('Content-Disposition: attachment; filename="node_import-'. strtr($task['type'], ':', '_') .'-'. $task['taskid'] .'.txt"');

  print "------------------------------------------------------------\n";
  print "Drupal and PHP version:\n";
  print "------------------------------------------------------------\n";
  print "Drupal version: ". VERSION ." (". DRUPAL_CORE_COMPATIBILITY .")\n";
  print "PHP version: ". PHP_VERSION ."\n";
  print "\n";

  print "------------------------------------------------------------\n";
  print "Enabled modules and versions:\n";
  print "------------------------------------------------------------\n";
  foreach (module_rebuild_cache() as $name => $module) {
    if ($module->status && $module->type == 'module') {
      print $name ." ". $module->info['version'] ." (". $module->info['project'] .")\n";
    }
  }
  print "\n";

  print "------------------------------------------------------------\n";
  print "User permissions:\n";
  print "------------------------------------------------------------\n";
  if ($task['uid'] == 1) {
    print "User #1 has all privileges.\n";
  }
  else {
    $account = user_load(array('uid' => $task['uid']));
    $result = db_query("SELECT p.perm FROM {role} AS r INNER JOIN {permission} AS p ON p.rid = r.rid WHERE r.rid IN (". db_placeholders($account->roles) .")", array_keys($account->roles));
    $perms = array();
    while (($row = db_fetch_object($result))) {
      $perms += array_flip(explode(', ', $row->perm));
    }
    print implode("\n", array_keys($perms)) ."\n";
  }
  print "\n";

  print "------------------------------------------------------------\n";
  print "Task details:\n";
  print "------------------------------------------------------------\n";
  print '$task = '; var_dump($task);
  $fields = node_import_fields($task['type']);
  print '$fields = '; var_dump($fields);
  print "\n";

  print "------------------------------------------------------------\n";
  print "Errors and data from last ". variable_get('node_import:debug:row_count', 5) ." rows with errors:\n";
  print "------------------------------------------------------------\n";
  $result = db_query("SELECT * FROM {node_import_status} WHERE taskid = %d AND status = %d LIMIT %d", $task['taskid'], NODE_IMPORT_STATUS_ERROR, variable_get('node_import:debug:row_count', 5));
  $i = 0;
  while (($row = db_fetch_object($result))) {
    $i++;
    print "\n";
    print "----- Row $i -------------------------------------------------\n";
    $row->errors = unserialize($row->errors);
    print '$row_status = '; var_dump($row);

    list($file_offset, $data) = node_import_read_from_file($task['file']->filepath, $row->file_offset, $task['file_options']);
    print '$data = '; var_dump($data);
  }
  print "\n";

  exit();
}

