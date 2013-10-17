<?php
// $Id$

/**
 * @file
 * Views module extension to link 'node' table with 'node_user_access' table
 * Also defines relationship with 'node' and 'users' table in order to
 * built complex reports in Views.
 */

/**
 * @defgroup views_user_access_module user_access.module
 *
 * @{
 */

/**
 * Implementation of hook_views_data()
 */

function user_access_views_data() {
  // Basic table information.

  // ----------------------------------------------------------------
  // node_user_access table

  $data['node_user_access']['table']['group']  = t('Users node views statistics');

  // Advertise this table as a possible base table
  $data['node_user_access']['table']['base'] = array(
    'field' => 'aid',
    'title' => t('Node views'),
    'help' => t("Node views statistics"),
    'weight' => 11,
  );

  // Describe relations with other core tables
  $data['node_user_access']['table']['join'] = array
  (
    // ...to the node table
    'node' => array(
      'left_field' => 'nid',
      'field' => 'nid',
    ),
    // ...to the users table
    'users' => array(
      'left_field' => 'uid',
      'field' => 'uid',
    ),
  );

  // aid (for count purpose, see 'Views Group By' module)
  $data['node_user_access']['aid'] = array
  (
    'title' => t('User\'s node view ID'),
    'help' => t('User\'s node view ID. Considere using this field with \'Views Group By\' module'),

    'field' => array
    (
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array
    (
      'handler' => 'views_handler_filter',
    ),
    'sort' => array
    (
      'handler' => 'views_handler_sort',
    ),
  );

  // node
  $data['node_user_access']['nid'] = array
  (
    'title' => t('Node'),
    'help' => t('Viewed node ID'),
    'relationship' => array
    (
      'base' => 'node',
      'base field' => 'nid',
      'handler' => 'views_handler_relationship',
      'label' => t('Node'),
    ),
  );

  // user
  $data['node_user_access']['uid'] = array
  (
    'title' => t('User'),
    'help' => t('Viewer\'s user ID'),
    'relationship' => array
    (
      'handler' => 'views_handler_relationship',
      'base' => 'users',
      'base field' => 'uid',
      'label' => t('User'),
    ),
  );

  
  // timestamp
  $data['node_user_access']['timestamp'] = array
  (
    'title' => t('View time'),
    'help' => t('Date and time the node was viewed'),

    'field' => array
    (
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'filter' => array
    (
      'handler' => 'views_handler_filter_date',
    ),
    'sort' => array
    (
      'handler' => 'views_handler_sort',
    ),
  );
  

  return $data;
}

/**
 * Implementation of hook_views_data_alter()
 *
 * Alter 'node' and 'users' data declarations in order users' node views may 
 * be used when creating views based on either 'node' or "users' tables.
 */
function user_access_views_data_alter(&$data) 
{
  $data['node']['table']['join']['node_user_access'] = array
  (
    'left_field' => 'nid',
    'field' => 'nid',
  );

  $data['users']['table']['join']['node_user_access'] = array
  (
    'left_field' => 'uid',
    'field' => 'uid',
  );
}
/**
 * @}
 */
