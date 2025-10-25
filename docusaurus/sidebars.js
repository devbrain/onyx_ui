// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  // By default, Docusaurus generates a sidebar from the docs folder structure
  tutorialSidebar: [
    'intro',
    {
      type: 'category',
      label: 'Core Concepts',
      items: [
        'core-concepts/backend-pattern',
        'core-concepts/two-pass-layout',
        'core-concepts/render-context',
        'core-concepts/background-renderer',
        'core-concepts/event-system',
        'core-concepts/theming-system',
        'core-concepts/service-locator',
        'core-concepts/focus-manager',
        'core-concepts/layer-manager',
        'core-concepts/hotkeys-manager',
        'core-concepts/ui-context',
        'core-concepts/ui-handle',
      ],
    },
    {
      type: 'category',
      label: 'Guides',
      items: [
        'guides/getting-started',
        'guides/layout-system',
        'guides/creating-custom-widgets',
      ],
    },
    {
      type: 'category',
      label: 'API Reference',
      items: [
        'api-reference/widget-library',
        {
          type: 'category',
          label: 'Widgets',
          items: [
            'api-reference/widgets/button',
            'api-reference/widgets/label',
            'api-reference/widgets/panel',
            'api-reference/widgets/vbox-hbox',
            'api-reference/widgets/grid',
          ],
        },
      ],
    },
  ],
};

export default sidebars;
