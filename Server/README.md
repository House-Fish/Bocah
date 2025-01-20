# Server
The server has two primary functions
1. Aggregation of logs from Client 
2. Sending out Monthly Reports every month 

# Event Tracking API Specification

## Overview
This API handles event tracking for different types of activities, specifically transportation and air conditioning usage events.

## Base URL
`https://api.example.com/v1`

## Endpoints

### POST /events
Records a new event occurrence.

#### Request Headers
- `Content-Type: application/json`

#### Request Body

The request body should be a JSON object conforming to one of the following schemas:

**Transportation Event Schema**
```json
{
  "type": "transportation",
  "device_id": "string",
  "mode": "string",
  "duration": "number",
  "message": "string"
}
```

**Air Conditioner Event Schema**
```json
{
  "type": "air-conditioner",
  "device_id": "string",
  "duration": "number",
  "message": "string"
}
```

#### Field Descriptions

**Common Fields:**
- `device_id` (required): Unique identifier for the device sending the event
  - Format: UUID v4
  - Example: "550e8400-e29b-41d4-a716-446655440000"
- `duration` (required): Duration of the event in seconds
  - Type: integer
  - Example: 3600 (for 1 hour)

**Transportation Event Fields:**
- `type` (required): Must be "transportation"
- `mode` (required): The mode of transportation
  - Allowed values: "car", "bike", "walk", "bus", "train"
- `message` (required): Description of the event

**Air Conditioner Event Fields:**
- `type` (required): Must be "air-conditioner"
- `message` (required): Description of the event

#### Response

**Success Response (200 OK)**
```json
{
  "success": true,
  "eventId": "string",
  "deviceId": "string",
  "timestamp": "string"
}
```

**Error Responses**

*400 Bad Request*
```json
{
  "error": "Bad Request",
  "message": "Invalid request payload",
  "details": [
    {
      "field": "string",
      "error": "string"
    }
  ]
}
```

## Example Requests

**Transportation Event**
```curl
curl -X POST https://api.example.com/v1/events \
  -H "Content-Type: application/json" \
  -d '{
    "type": "transportation",
    "device_id": "550e8400-e29b-41d4-a716-446655440000",
    "mode": "car",
    "duration": 1800,
    "message": "Mode change detected"
  }'
```

**Air Conditioner Event**
```curl
curl -X POST https://api.example.com/v1/events \
  -H "Content-Type: application/json" \
  -d '{
    "type": "air-conditioner",
    "device_id": "550e8400-e29b-41d4-a716-446655440000",
    "duration": 3600,
    "message": "Mode change detected"
  }'
```