module.exports = {
  root: true,
  env: {
    node: true,
  },
  extends: [
    'plugin:vue/vue3-essential',
    '@vue/eslint-config-airbnb-with-typescript',
  ],
  rules: {
    'vue/max-attributes-per-line': 'off',
    'import/no-unresolved': 'off',
    '@typescript-eslint/no-unused-vars': 'off',
    'import/order': 'off',
    'no-continue': 'off',
  },
  overrides: [
    {
      files: ['*.vue'],
      rules: {
        'no-console': 'off',
        'vue/max-attributes-per-line': ['error', {
          singleline: {
            max: 50,
          },
          multiline: {
            max: 1,
          },
        }],
        'vue/multi-word-component-names': 'off',
        'react/jsx-wrap-multilines': 'off',
        'vue/object-curly-newline': 'off',
        'react/jsx-curly-newline': 'off',
        'import/newline-after-import': 'off',
        'array-element-newline': 'off',
        'object-curly-newline': 'off',
        'vue/no-dupe-keys': 'off',
        'vue/no-setup-props-destructure': 'off',
        'vue/no-v-text-v-html-on-component': 'off',
        'vue/max-len': ['error',
          {
            code: 300,
            template: 300,
            comments: 300,
            ignoreTemplateLiterals: true,
            ignoreUrls: true,
            ignoreStrings: true,
          },
        ],
      },
    },
  ],
};
