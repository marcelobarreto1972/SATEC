from fastapi import FastAPI, HTTPException, Depends
from sqlalchemy.orm import Session
from typing import List
from datetime import datetime

from database import SessionLocal, engine, Base
from models import SensorReading
from schemas import SensorDataInput, SensorDataResponse

Base.metadata.create_all(bind=engine)

app = FastAPI(title="Sensor Data API", version="1.0.0")


def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()


@app.get("/health")
def health_check():
    return {"status": "ok"}


@app.post("/sensor-data", response_model=List[SensorDataResponse], status_code=201)
def ingest_sensor_data(payload: SensorDataInput, db: Session = Depends(get_db)):
    """Receive an array of temperature and humidity readings and persist them."""
    if not payload.readings:
        raise HTTPException(status_code=400, detail="readings array must not be empty")

    records = []
    for item in payload.readings:
        record = SensorReading(
            temperature=item.temperature,
            humidity=item.humidity,
            recorded_at=item.recorded_at or datetime.utcnow(),
        )
        db.add(record)
        records.append(record)

    db.commit()
    for r in records:
        db.refresh(r)

    return records


@app.get("/sensor-data", response_model=List[SensorDataResponse])
def list_sensor_data(limit: int = 100, db: Session = Depends(get_db)):
    """Return the most recent sensor readings."""
    return (
        db.query(SensorReading)
        .order_by(SensorReading.recorded_at.desc())
        .limit(limit)
        .all()
    )