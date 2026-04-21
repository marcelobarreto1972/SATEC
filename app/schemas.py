from pydantic import BaseModel, Field
from typing import List, Optional
from datetime import datetime


class SensorReading(BaseModel):
    temperature: float = Field(..., description="Temperature in degrees Celsius")
    humidity: float = Field(..., ge=0, le=100, description="Relative humidity percentage (0–100)")
    recorded_at: Optional[datetime] = Field(None, description="ISO-8601 timestamp; defaults to now")


class SensorDataInput(BaseModel):
    readings: List[SensorReading]


class SensorDataResponse(BaseModel):
    id: int
    temperature: float
    humidity: float
    recorded_at: datetime
    created_at: datetime

    class Config:
        from_attributes = True