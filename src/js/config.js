module.exports = [
  {
    "type": "heading",
    "id": "main-heading",
    "defaultValue": "Plain View Configuration",
    "size": 1
  },
  
  {
    "type": "text",
    "defaultValue": "Use This configuration page to adjust settings."
  },
  
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather"
      },
      {
        "type": "radiogroup",
        "messageKey": "TempUnits",
        "label": "Units",
        "defaultValue": "0",
        "options": [
          {
            "label": "Fahrenheit",
            "value": "0"
          },
          {
            "label": "Celcius",
            "value": "1"
          }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];