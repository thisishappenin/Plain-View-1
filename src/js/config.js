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
        "type": "select",
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
      },
      {
        "type": "select",
        "messageKey": "WeatherUpdateRate",
        "label": "Weather Update rate",
        "defaultValue": '2',
        "options": [
          {
            "label": "Manual Update When Expired",
            "value": "0"
          },
          {
            "label": "10 Minutes",
            "value": "1"
          },
          {
            "label": "20 Minutes",
            "value": "2"
          },
          {
            "label": "30 Minutes",
            "value": "3"
          },
          {
            "label": "60 Minutes",
            "value": "4"
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